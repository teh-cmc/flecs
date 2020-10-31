#include "private_api.h"

typedef struct sparse_t {
    int32_t dense;
} sparse_t;

void _ecs_dense_init(
    ecs_dense_t *d,
    ecs_size_t size)
{    
    d->size = ecs_to_i16(size);

    /* If size is non-zero, allocate extra column in dense array */
    if (size) {
        ecs_paged_init(&d->dense, 2, (ecs_size_t[]){ECS_SIZEOF(uint64_t), size});
    } else {
        ecs_paged_init(&d->dense, 1, (ecs_size_t[]){ECS_SIZEOF(uint64_t)});
    }

    /* Consume first value in dense array as 0 is used in the sparse array to
     * indicate that a sparse element hasn't been paired yet. */
    ecs_paged_add(&d->dense);

    ecs_paged_init(&d->sparse, 1, &(ecs_size_t){ECS_SIZEOF(sparse_t)});    
}

ecs_dense_t* _ecs_dense_new(
    ecs_size_t size)
{
    ecs_dense_t *d = ecs_os_calloc(ECS_SIZEOF(ecs_dense_t));
    ecs_assert(d != NULL, ECS_OUT_OF_MEMORY);

    _ecs_dense_init(d, size);

    return d;
}

int32_t ecs_dense_count(
    const ecs_dense_t *d)
{
    if (!d) {
        return 0;
    }

    return ecs_paged_count(&d->dense) - 1;
}

int32_t ecs_dense_size(
    const ecs_dense_t *d)
{
    if (!d) {
        return 0;
    }

    return ecs_paged_count(&d->dense) - 1;
}

void* _ecs_dense_get(
    const ecs_dense_t *d,
    ecs_size_t size,
    uint64_t index)
{
    ecs_assert(d != NULL, ECS_INVALID_PARAMETER);
    ecs_assert(!size || d->size == size, ECS_INVALID_PARAMETER);

    sparse_t *sparse = ecs_paged_get(&d->sparse, sparse_t, (int32_t)index, 0);
    int32_t dense_index;
    if (!sparse || !(dense_index = sparse->dense)) {
        return NULL;
    }

    size = d->size;
    return _ecs_paged_get(&d->dense, size, dense_index, 1);
}

struct ecs_dense_ensure_get_t _ecs_dense_ensure_get(
    ecs_dense_t *d,
    ecs_size_t size,
    uint64_t index)
{
    ecs_assert(d != NULL, ECS_INVALID_PARAMETER);
    ecs_assert(!size || d->size == size, ECS_INVALID_PARAMETER);
    ecs_assert(ecs_paged_count(&d->dense) > 0, ECS_INTERNAL_ERROR);

    sparse_t *sparse = ecs_paged_ensure(&d->sparse, sparse_t, (int32_t)index, 0);
    ecs_assert(sparse != NULL, ECS_INTERNAL_ERROR);
    int32_t dense = sparse->dense;

    size = d->size;

    if (!dense) {
        dense = ecs_paged_add(&d->dense);
        uint64_t *dense_elem = ecs_paged_get(&d->dense, uint64_t, dense, 0);
        dense_elem[0] = index;
        sparse->dense = dense;
        return (struct ecs_dense_ensure_get_t) {
            .ptr = _ecs_paged_get(&d->dense, size, dense, 1),
            .added = true
        };
    }
  
    return (struct ecs_dense_ensure_get_t) {
        .ptr = _ecs_paged_get(&d->dense, size, dense, 1),
        .added = false
    };
}

bool _ecs_dense_ensure(
    ecs_dense_t *d,
    ecs_size_t size,
    uint64_t index)
{
    ecs_assert(d != NULL, ECS_INVALID_PARAMETER);
    ecs_assert(!size || d->size == size, ECS_INVALID_PARAMETER);
    ecs_assert(ecs_paged_count(&d->dense) > 0, ECS_INTERNAL_ERROR);

    sparse_t *sparse = ecs_paged_ensure(&d->sparse, sparse_t, (int32_t)index, 0);
    ecs_assert(sparse != NULL, ECS_INTERNAL_ERROR);
    int32_t dense = sparse->dense;

    if (!dense) {
        dense = ecs_paged_add(&d->dense);
        uint64_t *dense_elem = ecs_paged_get(&d->dense, uint64_t, dense, 0);
        dense_elem[0] = index;
        sparse->dense = dense;
        return true;
    }

    return false;
}

bool ecs_dense_remove(
    ecs_dense_t *d,
    uint64_t index)
{
    ecs_assert(d != NULL, ECS_INVALID_PARAMETER);

    sparse_t *sparse = ecs_paged_get(&d->sparse, sparse_t, (int32_t)index, 0);
    if (!sparse) {
        return false;
    }

    int32_t dense = sparse->dense;

    if (dense) {
        int32_t count = ecs_paged_count(&d->dense);
        uint64_t *last_dense = ecs_paged_get(&d->dense, uint64_t, count - 1, 0);
        sparse_t *last_sparse = ecs_paged_get(
            &d->sparse, sparse_t, (int32_t)last_dense[0], 0);
        last_sparse->dense = dense;
        sparse->dense = 0;
        ecs_paged_remove(&d->dense, dense);
        return true;
    }

    return false;
}

void ecs_dense_clear(
    ecs_dense_t *d)
{
    ecs_assert(d != NULL, ECS_INVALID_PARAMETER);
    ecs_paged_deinit(&d->sparse);
    ecs_paged_deinit(&d->dense);
}

void ecs_dense_fini(
    ecs_dense_t *d)
{
    ecs_dense_clear(d);
}

void ecs_dense_free(
    ecs_dense_t *d)
{
    ecs_dense_fini(d);
    ecs_os_free(d);
}
