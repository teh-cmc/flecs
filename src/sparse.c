#include "private_api.h"

#define SPARSE  (0)
#define PAYLOAD (1)

struct ecs_sparse_t {
    ecs_vector_t *dense;        /* Dense array with indices to sparse array. The
                                 * dense array stores both alive and not alive
                                 * sparse indices. The 'count' member keeps
                                 * track of which indices are alive. */

    ecs_paged_t sparse;         /* Sparse array & sparse payload */
    ecs_size_t size;            /* Element size */
    int32_t count;              /* Number of alive entries */
    uint64_t max_id_local;      /* Local max index (if no global is set) */
    uint64_t *max_id;           /* Maximum issued sparse index */
};

static
void sparse_grow_dense(
    ecs_sparse_t *sparse)
{
    ecs_vector_add(&sparse->dense, uint64_t);
}

static
uint64_t strip_generation(
    uint64_t *id_without_generation)
{
    uint64_t index = *id_without_generation;
    uint64_t gen = index & ECS_GENERATION_MASK;
    *id_without_generation -= gen;

    /* The resulting index must be smaller than UINT32_MAX because after 
     * stripping the generation count no bits should be set in the upper 64
     * bits of the id. If there are still bits set, this is likely because the
     * application tried to lookup a role as an entity */
    ecs_assert(*id_without_generation < UINT32_MAX, ECS_INVALID_PARAMETER);
    return gen;
}

static
void sparse_assign_index(
    int32_t * sparse_elem, 
    uint64_t * dense_array, 
    uint64_t index, 
    int32_t dense)
{
    sparse_elem[0] = dense;
    dense_array[dense] = index;
}

static
uint64_t inc_gen(
    uint64_t index)
{
    return ECS_GENERATION_INC(index);
}

static
uint64_t inc_id(
    ecs_sparse_t *sparse)
{
    return ++ (sparse->max_id[0]);
}

static
uint64_t get_id(
    const ecs_sparse_t *sparse)
{
    return sparse->max_id[0];
}

static
void set_id(
    ecs_sparse_t *sparse,
    uint64_t value)
{
    sparse->max_id[0] = value;
}

static
int32_t* sparse_ensure_sparse(
    ecs_sparse_t *sparse,
    uint64_t index)
{
    int32_t *elem = ecs_paged_ensure(
        &sparse->sparse, int32_t, ecs_to_i32(index), SPARSE);
    ecs_assert(elem != NULL, ECS_INTERNAL_ERROR);
    return elem;
}

static
int32_t* get_sparse(
    const ecs_sparse_t *sparse,
    uint64_t index)
{
    return ecs_paged_get(&sparse->sparse, int32_t, ecs_to_i32(index), SPARSE);
}

static
void* get_payload(
    const ecs_sparse_t *sparse,
    int32_t dense,
    uint64_t index)
{
    ecs_assert(!dense || dense == ecs_paged_get(&sparse->sparse, int32_t, 
        ecs_to_i32(index), SPARSE)[0], 
        ECS_INTERNAL_ERROR);

    return _ecs_paged_get(&sparse->sparse, sparse->size, 
        ecs_to_i32(index), PAYLOAD);
}

static
void* page_get_payload(
    const ecs_sparse_t *sparse,
    ecs_page_t *page,
    uint64_t index)
{
    return _ecs_paged_page_get(&sparse->sparse, page, sparse->size, 
        ecs_to_i32(index), PAYLOAD);
}

static
int32_t* page_get_sparse(
    const ecs_sparse_t *sparse,
    ecs_page_t *page,
    uint64_t index)
{
    return ecs_paged_page_get(&sparse->sparse, page, int32_t, 
        (int32_t)index, SPARSE);
}

static
int32_t page_get_sparse_value(
    const ecs_sparse_t *sparse,
    ecs_page_t *page,
    uint64_t index)
{
    int32_t *ptr = page_get_sparse(sparse, page, index);
    ecs_assert(ptr != NULL, ECS_INTERNAL_ERROR);
    return ptr[0];
}

static
uint64_t create_id(
    ecs_sparse_t *sparse,
    int32_t dense)
{   
    sparse_grow_dense(sparse);
    uint64_t index = inc_id(sparse);
    uint64_t *dense_array = ecs_vector_first(sparse->dense, uint64_t);
    int32_t *sparse_elem = sparse_ensure_sparse(sparse, index);
    sparse_assign_index(sparse_elem, dense_array, index, dense);
    return index;
}

static
uint64_t new_index(
    ecs_sparse_t *sparse)
{
    ecs_vector_t *dense = sparse->dense;
    int32_t dense_count = ecs_vector_count(dense);
    int32_t count = sparse->count ++;

    ecs_assert(count <= dense_count, ECS_INTERNAL_ERROR);

    if (count < dense_count) {
        /* If there are unused elements in the dense array, return first */
        uint64_t *dense_array = ecs_vector_first(dense, uint64_t);
        return dense_array[count];
    } else {
        return create_id(sparse, count);
    }
}

static
void* try_sparse_any(
    const ecs_sparse_t *sparse,
    uint64_t index)
{
    strip_generation(&index);
    ecs_page_t *page = ecs_paged_get_page(&sparse->sparse, ecs_to_i32(index));
    if (!page) {
        return NULL;
    }

    int32_t dense = page_get_sparse_value(sparse, page, index);
    bool in_use = dense && (dense < sparse->count);
    if (!in_use) {
        return NULL;
    }

    return page_get_payload(sparse, page, index);
}

static
void* try_sparse(
    const ecs_sparse_t *sparse,
    uint64_t index)
{
    ecs_page_t *page = ecs_paged_get_page(&sparse->sparse, (int32_t)index);
    if (!page) {
        return NULL;
    }

    int32_t dense = page_get_sparse_value(sparse, page, index);
    bool in_use = dense && (dense < sparse->count);
    if (!in_use) {
        return NULL;
    }

    uint64_t gen = strip_generation(&index);
    uint64_t *dense_array = ecs_vector_first(sparse->dense, uint64_t);
    uint64_t cur_gen = dense_array[dense] & ECS_GENERATION_MASK;

    if (cur_gen != gen) {
        return NULL;
    }

    return page_get_payload(sparse, page, index);
}

static
void sparse_swap_dense(
    ecs_sparse_t * sparse,
    ecs_page_t * page_a,
    int32_t a,
    int32_t b)
{
    ecs_assert(a != b, ECS_INTERNAL_ERROR);
    uint64_t *dense_array = ecs_vector_first(sparse->dense, uint64_t);
    uint64_t index_a = dense_array[a];
    uint64_t index_b = dense_array[b];

    int32_t *sparse_a = page_get_sparse(sparse, page_a, (uint32_t)index_a);
    int32_t *sparse_b = sparse_ensure_sparse(sparse, (uint32_t)index_b);

    sparse_assign_index(sparse_a, dense_array, index_a, b);
    sparse_assign_index(sparse_b, dense_array, index_b, a);
}

ecs_sparse_t* _ecs_sparse_new(
    ecs_size_t size)
{
    ecs_sparse_t *result = ecs_os_calloc(ECS_SIZEOF(ecs_sparse_t));
    ecs_assert(result != NULL, ECS_OUT_OF_MEMORY);
    result->size = size;
    result->max_id_local = UINT64_MAX;
    result->max_id = &result->max_id_local;

    ecs_paged_init(&result->sparse, 2, (ecs_size_t[]){
        ECS_SIZEOF(int32_t), size
    });

    /* Consume first value in dense array as 0 is used in the sparse array to
     * indicate that a sparse element hasn't been paired yet. */
    ecs_vector_add(&result->dense, uint64_t);
    result->count = 1;
    return result;
}

void ecs_sparse_set_id_source(
    ecs_sparse_t * sparse,
    uint64_t * id_source)
{
    ecs_assert(sparse != NULL, ECS_INVALID_PARAMETER);
    sparse->max_id = id_source;
}

void ecs_sparse_clear(
    ecs_sparse_t *sparse)
{
    ecs_assert(sparse != NULL, ECS_INVALID_PARAMETER);

    ecs_paged_clear(&sparse->sparse);

    ecs_vector_set_count(&sparse->dense, uint64_t, 1);

    sparse->count = 1;
    sparse->max_id_local = 0;
}

void ecs_sparse_free(
    ecs_sparse_t *sparse)
{
    if (sparse) {
        ecs_sparse_clear(sparse);
        ecs_paged_deinit(&sparse->sparse);
        ecs_vector_free(sparse->dense);
        ecs_os_free(sparse);
    }
}

uint64_t ecs_sparse_new_id(
    ecs_sparse_t *sparse)
{
    ecs_assert(sparse != NULL, ECS_INVALID_PARAMETER);
    return new_index(sparse);
}

const uint64_t* ecs_sparse_new_ids(
    ecs_sparse_t *sparse,
    int32_t new_count)
{
    ecs_assert(sparse != NULL, ECS_INVALID_PARAMETER);
    int32_t dense_count = ecs_vector_count(sparse->dense);
    int32_t count = sparse->count;
    int32_t remaining = dense_count - count;
    int32_t i, to_create = new_count - remaining;

    if (to_create > 0) {
        ecs_sparse_set_size(sparse, dense_count + to_create);
        uint64_t *dense_array = ecs_vector_first(sparse->dense, uint64_t);

        for (i = 0; i < to_create; i ++) {
            uint64_t index = create_id(sparse, count + i);
            dense_array[dense_count + i] = index;
        }
    }

    sparse->count += new_count;

    return ecs_vector_get(sparse->dense, uint64_t, count);
}

void* _ecs_sparse_add(
    ecs_sparse_t *sparse,
    ecs_size_t size)
{
    ecs_assert(sparse != NULL, ECS_INVALID_PARAMETER);
    ecs_assert(!size || size == sparse->size, ECS_INVALID_PARAMETER);
    uint64_t index = new_index(sparse);
    return get_payload(sparse, 0, index);
}

uint64_t ecs_sparse_last_id(
    ecs_sparse_t *sparse)
{
    ecs_assert(sparse != NULL, ECS_INTERNAL_ERROR);
    uint64_t *dense_array = ecs_vector_first(sparse->dense, uint64_t);
    return dense_array[sparse->count - 1];
}

static
void* get_or_create(
    ecs_sparse_t *sparse,
    uint64_t index,
    uint64_t gen,
    ecs_page_t *page,
    int32_t dense)
{
    if (dense) {
        /* Check if element is alive. If element is not alive, update indices so
         * that the first unused dense element points to the sparse element. */
        int32_t count = sparse->count;
        if (dense == count) {
            /* If dense is the next unused element in the array, simply increase
             * the count to make it part of the alive set. */
            sparse->count ++;
        } else if (dense > count) {
            /* If dense is not alive, swap it with the first unused element. */
            sparse_swap_dense(sparse, page, dense, count);

            /* First unused element is now last used element */
            sparse->count ++;
        } else {
            /* Dense is already alive, nothing to be done */
        }
    } else {
        /* Element is not paired yet. Must add a new element to dense array */
        sparse_grow_dense(sparse);

        ecs_vector_t *dense_vector = sparse->dense;
        uint64_t *dense_array = ecs_vector_first(dense_vector, uint64_t);
        int32_t dense_count = ecs_vector_count(dense_vector) - 1;
        int32_t count = sparse->count ++;

        /* If index is larger than max id, update max id */
        uint64_t max_id = get_id(sparse);
        if ((max_id == (uint64_t)-1) || (index >= get_id(sparse))) {
            set_id(sparse, index);
        }

        if (count < dense_count) {
            /* If there are unused elements in the list, move the first unused
             * element to the end of the list */
            uint64_t unused = dense_array[count];
            strip_generation(&unused);
            int32_t *unused_sparse = sparse_ensure_sparse(sparse, unused);
            sparse_assign_index(unused_sparse, dense_array, unused, dense_count);
        }

        int32_t *sparse_elem = page_get_sparse(sparse, page, index);
        ecs_assert(sparse_elem != NULL, ECS_INTERNAL_ERROR);
        sparse_assign_index(sparse_elem, dense_array, index, count);
        dense_array[count] |= gen;
    }

    return page_get_payload(sparse, page, index);
}

void* _ecs_sparse_get_or_create(
    ecs_sparse_t *sparse,
    ecs_size_t size,
    uint64_t index)
{
    ecs_assert(sparse != NULL, ECS_INVALID_PARAMETER);
    ecs_assert(!size || size == sparse->size, ECS_INVALID_PARAMETER);
    ecs_assert(ecs_vector_count(sparse->dense) > 0, ECS_INTERNAL_ERROR);

    uint64_t gen = strip_generation(&index);
    ecs_page_t *page = ecs_paged_ensure_page(&sparse->sparse, ecs_to_i32(index));
    ecs_assert(page != NULL, ECS_INTERNAL_ERROR);
    
    int32_t *dense = page_get_sparse(sparse, page, index);
    ecs_assert(dense != NULL, ECS_INTERNAL_ERROR);

    return get_or_create(sparse, index, gen, page, dense[0]);
}

static
void* remove_get(
    ecs_sparse_t *sparse,
    uint64_t index,
    uint64_t gen,
    ecs_page_t *page,
    int32_t dense)    
{
    if (dense) {
        uint64_t *dense_array = ecs_vector_first(sparse->dense, uint64_t);
        uint64_t cur_gen = dense_array[dense] & ECS_GENERATION_MASK;
        if (gen != cur_gen) {
            /* Generation doesn't match which means that the provided entity is
             * already not alive. */
            return NULL;
        }

        /* Increase generation */
        dense_array[dense] = index | inc_gen(cur_gen);
        
        int32_t count = sparse->count;
        if (dense == (count - 1)) {
            /* If dense is the last used element, simply decrease count */
            sparse->count --;
        } else if (dense < count) {
            /* If element is alive, move it to unused elements */
            sparse_swap_dense(sparse, page, dense, count - 1);
            sparse->count --;
        } else {
            /* Element is not alive, nothing to be done */
            return NULL;
        }

        /* Reset memory to zero on remove */
        return page_get_payload(sparse, page, index);
    } else {
        /* Element is not paired and thus not alive, nothing to be done */
        return NULL;
    }
}

void* _ecs_sparse_remove_get(
    ecs_sparse_t *sparse,
    ecs_size_t size,
    uint64_t index)
{
    ecs_assert(sparse != NULL, ECS_INVALID_PARAMETER);
    ecs_assert(!size || size == sparse->size, ECS_INVALID_PARAMETER);
    ecs_page_t *page = ecs_paged_get_page(&sparse->sparse, (int32_t)index);
    if (!page) {
        return NULL;
    }

    uint64_t gen = strip_generation(&index);
    int32_t dense = page_get_sparse_value(sparse, page, index);

    return remove_get(sparse, index, gen, page, dense);
}

void ecs_sparse_remove(
    ecs_sparse_t *sparse,
    uint64_t index)
{
    void *ptr = _ecs_sparse_remove_get(sparse, 0, index);
    if (ptr) {
        ecs_os_memset(ptr, 0, sparse->size);
    }
}

void ecs_sparse_set_generation(
    ecs_sparse_t *sparse,
    uint64_t index)
{
    ecs_assert(sparse != NULL, ECS_INVALID_PARAMETER);

    int32_t *dense_ptr = sparse_ensure_sparse(sparse, index);
    ecs_assert(dense_ptr != NULL, ECS_INTERNAL_ERROR);
    int32_t dense = dense_ptr[0];

    if (dense) {
        /* Increase generation */
        uint64_t *dense_array = ecs_vector_first(sparse->dense, uint64_t);
        dense_array[dense] = index;
    } else {
        /* Element is not paired and thus not alive, nothing to be done */
    }
}

bool ecs_sparse_exists(
    const ecs_sparse_t *sparse,
    uint64_t index)
{
    ecs_assert(sparse != NULL, ECS_INVALID_PARAMETER);
    
    int32_t *dense_ptr = get_sparse(sparse, index);
    if (!dense_ptr) {
        return false;
    }

    int32_t dense = dense_ptr[0];
    return (dense != 0) && (dense < sparse->count);
}

void* _ecs_sparse_get_elem(
    const ecs_sparse_t *sparse,
    ecs_size_t size,
    int32_t dense_index)
{
    ecs_assert(sparse != NULL, ECS_INVALID_PARAMETER);
    ecs_assert(!size || size == sparse->size, ECS_INVALID_PARAMETER);
    ecs_assert(dense_index < sparse->count, ECS_INVALID_PARAMETER);

    dense_index ++;

    uint64_t *dense_array = ecs_vector_first(sparse->dense, uint64_t);
    return get_payload(sparse, dense_index, dense_array[dense_index]);
}

bool ecs_sparse_is_alive(
    const ecs_sparse_t *sparse,
    uint64_t index)
{
    return try_sparse(sparse, index) != NULL;
}

void* _ecs_sparse_get(
    const ecs_sparse_t *sparse,
    ecs_size_t size,
    uint64_t index)
{
    ecs_assert(sparse != NULL, ECS_INVALID_PARAMETER);
    ecs_assert(!size || size == sparse->size, ECS_INVALID_PARAMETER);
    return try_sparse(sparse, index);
}

void* _ecs_sparse_get_any(
    ecs_sparse_t *sparse,
    ecs_size_t size,
    uint64_t index)
{
    ecs_assert(sparse != NULL, ECS_INVALID_PARAMETER);
    ecs_assert(!size || size == sparse->size, ECS_INVALID_PARAMETER);
    return try_sparse_any(sparse, index);
}

int32_t ecs_sparse_count(
    const ecs_sparse_t *sparse)
{
    if (!sparse) {
        return 0;
    }

    return sparse->count - 1;
}

int32_t ecs_sparse_size(
    const ecs_sparse_t *sparse)
{
    if (!sparse) {
        return 0;
    }

    return ecs_vector_count(sparse->dense) - 1;
}

const uint64_t* ecs_sparse_ids(
    const ecs_sparse_t *sparse)
{
    ecs_assert(sparse != NULL, ECS_INVALID_PARAMETER);
    return &(ecs_vector_first(sparse->dense, uint64_t)[1]);
}

void ecs_sparse_set_size(
    ecs_sparse_t *sparse,
    int32_t elem_count)
{
    ecs_assert(sparse != NULL, ECS_INVALID_PARAMETER);
    ecs_vector_set_size(&sparse->dense, uint64_t, elem_count);
}

static
void sparse_copy(
    ecs_sparse_t * dst,
    const ecs_sparse_t * src)
{
    ecs_sparse_set_size(dst, ecs_sparse_size(src));
    const uint64_t *indices = ecs_sparse_ids(src);
    
    ecs_size_t size = src->size;
    int32_t i, count = src->count;

    for (i = 0; i < count - 1; i ++) {
        uint64_t index = indices[i];
        void *src_ptr = _ecs_sparse_get(src, size, index);
        void *dst_ptr = _ecs_sparse_get_or_create(dst, size, index);
        ecs_sparse_set_generation(dst, index);
        ecs_os_memcpy(dst_ptr, src_ptr, size);
    }

    set_id(dst, get_id(src));

    ecs_assert(src->count == dst->count, ECS_INTERNAL_ERROR);
}

ecs_sparse_t* ecs_sparse_copy(
    const ecs_sparse_t *src)
{
    if (!src) {
        return NULL;
    }

    ecs_sparse_t *dst = _ecs_sparse_new(src->size);
    sparse_copy(dst, src);

    return dst;
}

void ecs_sparse_restore(
    ecs_sparse_t * dst,
    const ecs_sparse_t * src)
{
    ecs_assert(dst != NULL, ECS_INVALID_PARAMETER);
    dst->count = 1;
    if (src) {
        sparse_copy(dst, src);
    }
}

void ecs_sparse_memory(
    const ecs_sparse_t *sparse,
    int32_t *allocd,
    int32_t *used)
{
    if (!sparse) {
        return;
    }

    *allocd += ECS_SIZEOF(ecs_sparse_t);

    // ecs_paged_memory(sparse->sparse, allocd, used);
    ecs_vector_memory(sparse->dense, uint64_t, allocd, used);
    
    // *allocd += ecs_vector_count(sparse->chunks) * 
    //     (sparse->size * CHUNK_COUNT + ECS_SIZEOF(int32_t) * CHUNK_COUNT);

    if (used) {
        *used += sparse->count * sparse->size;
    }
}
