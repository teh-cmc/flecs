#include "../private_api.h"
#include <math.h>

/* The ratio used to determine whether the map should rehash. If
 * (element_count * LOAD_FACTOR) > bucket_count, bucket count is increased. */
#define LOAD_FACTOR (1.5f)
#define KEY_SIZE (ECS_SIZEOF(ecs_map_key_t))
#define GET_ELEM(array, elem_size, index) \
    ECS_OFFSET(array, (elem_size) * (index))

static
uint8_t ecs_log2(uint32_t v) {
    static const uint8_t log2table[32] = 
        {0, 9,  1,  10, 13, 21, 2,  29, 11, 14, 16, 18, 22, 25, 3, 30,
         8, 12, 20, 28, 15, 17, 24, 7,  19, 27, 23, 6,  26, 5,  4, 31};

    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    return log2table[(uint32_t)(v * 0x07C4ACDDU) >> 27];
}

/* Get bucket count for number of elements */
static
int32_t get_bucket_count(
    int32_t element_count)
{
    return flecs_next_pow_of_2((int32_t)((float)element_count * LOAD_FACTOR));
}

/* Get bucket shift amount for a given bucket count */
static
uint8_t get_bucket_shift (
    int32_t bucket_count)
{
    return (uint8_t)(64u - ecs_log2((uint32_t)bucket_count));
}

/* Get bucket index for provided map key */
static
int32_t get_bucket_index(
    const ecs_map_t *map,
    uint16_t bucket_shift,
    ecs_map_key_t key) 
{
    ecs_assert(bucket_shift != 0, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(map->bucket_shift == bucket_shift, ECS_INTERNAL_ERROR, NULL);
    (void)map;
    return (int32_t)((11400714819323198485ull * key) >> bucket_shift);
}

/* Get bucket for key */
static
ecs_bucket_t* get_bucket(
    const ecs_map_t *map,
    ecs_map_key_t key)
{
    ecs_assert(map->bucket_shift == get_bucket_shift(map->bucket_count), 
        ECS_INTERNAL_ERROR, NULL);
    int32_t bucket_id = get_bucket_index(map, map->bucket_shift, key);
    ecs_assert(bucket_id < map->bucket_count, ECS_INTERNAL_ERROR, NULL);
    return &map->buckets[bucket_id];
}

/* Ensure that map has at least new_count buckets */
static
void ensure_buckets(
    ecs_map_t *map,
    int32_t new_count)
{
    int32_t bucket_count = map->bucket_count;
    new_count = flecs_next_pow_of_2(new_count);
    if (new_count < 2) {
        new_count = 2;
    }

    if (new_count && new_count > bucket_count) {
        map->buckets = ecs_os_realloc(map->buckets, new_count * ECS_SIZEOF(ecs_bucket_t));
        map->bucket_count = new_count;
        map->bucket_shift = get_bucket_shift(new_count);
        ecs_os_memset(
            ECS_OFFSET(map->buckets, bucket_count * ECS_SIZEOF(ecs_bucket_t)), 
            0, (new_count - bucket_count) * ECS_SIZEOF(ecs_bucket_t));
    }
}

/* Free contents of bucket */
static
void clear_bucket(
    ecs_bucket_t *bucket)
{
    ecs_os_free(bucket->keys);
    ecs_os_free(bucket->payload);
    bucket->keys = NULL;
    bucket->payload = NULL;
    bucket->count = 0;
}

/* Clear all buckets */
static
void clear_buckets(
    ecs_map_t *map)
{
    ecs_bucket_t *buckets = map->buckets;
    int32_t i, count = map->bucket_count;
    for (i = 0; i < count; i ++) {
        clear_bucket(&buckets[i]);
    }
    ecs_os_free(buckets);
    map->buckets = NULL;
    map->bucket_count = 0;
}

/* Find or create bucket for specified key */
static
ecs_bucket_t* ensure_bucket(
    ecs_map_t *map,
    ecs_map_key_t key)
{
    ecs_assert(map->bucket_count >= 2, ECS_INTERNAL_ERROR, NULL);
    int32_t bucket_id = get_bucket_index(map, map->bucket_shift, key);
    ecs_assert(bucket_id >= 0, ECS_INTERNAL_ERROR, NULL);
    return &map->buckets[bucket_id];
}

/* Add element to bucket */
static
int32_t add_to_bucket(
    ecs_bucket_t *bucket,
    ecs_size_t elem_size,
    ecs_map_key_t key,
    const void *payload)
{
    int32_t index = bucket->count ++;
    int32_t bucket_count = index + 1;

    bucket->keys = ecs_os_realloc(bucket->keys, KEY_SIZE * bucket_count);
    bucket->keys[index] = key;

    if (elem_size) {
        bucket->payload = ecs_os_realloc(bucket->payload, elem_size * bucket_count);
        if (payload) {
            void *elem = GET_ELEM(bucket->payload, elem_size, index);
            ecs_os_memcpy(elem, payload, elem_size);
        }
    } else {
        bucket->payload = NULL;
    }

    return index;
}

/*  Remove element from bucket */
static
void remove_from_bucket(
    ecs_bucket_t *bucket,
    ecs_size_t elem_size,
    ecs_map_key_t key,
    int32_t index)
{
    (void)key;

    ecs_assert(bucket->count != 0, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(index < bucket->count, ECS_INTERNAL_ERROR, NULL);
    
    int32_t bucket_count = -- bucket->count;

    if (index != bucket->count) {
        ecs_assert(key == bucket->keys[index], ECS_INTERNAL_ERROR, NULL);
        bucket->keys[index] = bucket->keys[bucket_count];

        ecs_map_key_t *elem = GET_ELEM(bucket->payload, elem_size, index);
        ecs_map_key_t *last_elem = GET_ELEM(bucket->payload, elem_size, bucket->count);

        ecs_os_memcpy(elem, last_elem, elem_size);
    }
}

/* Get payload pointer for key from bucket */
static
void* get_from_bucket(
    ecs_bucket_t *bucket,
    ecs_map_key_t key,
    ecs_size_t elem_size)
{
    ecs_map_key_t *keys = bucket->keys;
    int32_t i, count = bucket->count;

    for (i = 0; i < count; i ++) {
        if (keys[i] == key) {
            return GET_ELEM(bucket->payload, elem_size, i);
        }
    }
    return NULL;
}

/* Grow number of buckets */
static
void rehash(
    ecs_map_t *map,
    int32_t bucket_count)
{
    ecs_assert(bucket_count != 0, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(bucket_count > map->bucket_count, ECS_INTERNAL_ERROR, NULL);

    ensure_buckets(map, bucket_count);

    ecs_bucket_t *buckets = map->buckets;
    ecs_assert(buckets != NULL, ECS_INTERNAL_ERROR, NULL);
    ecs_size_t elem_size = map->elem_size;
    uint16_t bucket_shift = map->bucket_shift;
    int32_t bucket_id;

    /* Iterate backwards as elements could otherwise be moved to existing
     * buckets which could temporarily cause the number of elements in a
     * bucket to exceed BUCKET_COUNT. */
    for (bucket_id = bucket_count - 1; bucket_id >= 0; bucket_id --) {
        ecs_bucket_t *bucket = &buckets[bucket_id];

        int i, count = bucket->count;
        ecs_map_key_t *key_array = bucket->keys;
        void *payload_array = bucket->payload;

        for (i = 0; i < count; i ++) {
            ecs_map_key_t key = key_array[i];
            void *elem = GET_ELEM(payload_array, elem_size, i);
            int32_t new_bucket_id = get_bucket_index(map, bucket_shift, key);

            if (new_bucket_id != bucket_id) {
                ecs_bucket_t *new_bucket = &buckets[new_bucket_id];

                add_to_bucket(new_bucket, elem_size, key, elem);
                remove_from_bucket(bucket, elem_size, key, i);

                count --;
                i --;
            }
        }

        if (!bucket->count) {
            clear_bucket(bucket);
        }
    }
}

void _ecs_map_init(
    ecs_map_t *result,
    ecs_size_t elem_size,
    int32_t element_count)
{
    ecs_assert(elem_size < INT16_MAX, ECS_INVALID_PARAMETER, NULL);

    result->count = 0;
    result->elem_size = (int16_t)elem_size;

    ensure_buckets(result, get_bucket_count(element_count));
}

ecs_map_t* _ecs_map_new(
    ecs_size_t elem_size,
    int32_t element_count)
{
    ecs_map_t *result = ecs_os_calloc_t(ecs_map_t);
    ecs_assert(result != NULL, ECS_OUT_OF_MEMORY, NULL);

    _ecs_map_init(result, elem_size, element_count);

    return result;
}

bool ecs_map_is_initialized(
    const ecs_map_t *result)
{
    return result != NULL && result->bucket_count != 0;
}

void ecs_map_fini(
    ecs_map_t *map)
{
    ecs_assert(map != NULL, ECS_INTERNAL_ERROR, NULL);
    clear_buckets(map);
}

void ecs_map_free(
    ecs_map_t *map)
{
    if (map) {
        ecs_map_fini(map);
        ecs_os_free(map);
    }
}

void* _ecs_map_get(
    const ecs_map_t *map,
    ecs_size_t elem_size,
    ecs_map_key_t key)
{
    (void)elem_size;

    if (!ecs_map_is_initialized(map)) {
        return NULL;
    }

    ecs_assert(elem_size == map->elem_size, ECS_INVALID_PARAMETER, NULL);

    ecs_bucket_t * bucket = get_bucket(map, key);
    if (!bucket) {
        return NULL;
    }

    return get_from_bucket(bucket, key, elem_size);
}

void* _ecs_map_get_ptr(
    const ecs_map_t *map,
    ecs_map_key_t key)
{
    void* ptr_ptr = _ecs_map_get(map, ECS_SIZEOF(void*), key);

    if (ptr_ptr) {
        return *(void**)ptr_ptr;
    } else {
        return NULL;
    }
}

bool ecs_map_has(
    const ecs_map_t *map,
    ecs_map_key_t key)
{
    if (!ecs_map_is_initialized(map)) {
        return false;
    }

    ecs_bucket_t * bucket = get_bucket(map, key);
    if (!bucket) {
        return false;
    }

    return get_from_bucket(bucket, key, 0) != NULL;
}

void* _ecs_map_ensure(
    ecs_map_t *map,
    ecs_size_t elem_size,
    ecs_map_key_t key)
{
    void *result = _ecs_map_get(map, elem_size, key);
    if (!result) {
        result = _ecs_map_set(map, elem_size, key, NULL);
        if (elem_size) {
            ecs_assert(result != NULL, ECS_INTERNAL_ERROR, NULL);
            ecs_os_memset(result, 0, elem_size);
        }
    }

    return result;
}

void* _ecs_map_set(
    ecs_map_t *map,
    ecs_size_t elem_size,
    ecs_map_key_t key,
    const void *payload)
{
    ecs_assert(map != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_assert(elem_size == map->elem_size, ECS_INVALID_PARAMETER, NULL);

    ecs_bucket_t *bucket = ensure_bucket(map, key);
    ecs_assert(bucket != NULL, ECS_INTERNAL_ERROR, NULL);

    void *elem = get_from_bucket(bucket, key, elem_size);
    if (!elem) {
        int32_t index = add_to_bucket(bucket, elem_size, key, payload);        
        int32_t map_count = ++map->count;
        int32_t target_bucket_count = get_bucket_count(map_count);
        int32_t map_bucket_count = map->bucket_count;

        if (target_bucket_count > map_bucket_count) {
            rehash(map, target_bucket_count);
            bucket = ensure_bucket(map, key);
            return get_from_bucket(bucket, key, elem_size);
        } else {
            return GET_ELEM(bucket->payload, elem_size, index);
        }       
    } else {
        if (payload) {
            ecs_os_memcpy(elem, payload, elem_size);
        }
        return elem;
    }
}

int32_t ecs_map_remove(
    ecs_map_t *map,
    ecs_map_key_t key)
{
    ecs_assert(map != NULL, ECS_INVALID_PARAMETER, NULL);

    ecs_bucket_t * bucket = get_bucket(map, key);
    if (!bucket) {
        return map->count;
    }

    int32_t i, bucket_count = bucket->count;
    for (i = 0; i < bucket_count; i ++) {
        if (bucket->keys[i] == key) {
            remove_from_bucket(bucket, map->elem_size, key, i);
            return --map->count;
        }
    }

    return map->count;
}

int32_t ecs_map_count(
    const ecs_map_t *map)
{
    return map ? map->count : 0;
}

int32_t ecs_map_bucket_count(
    const ecs_map_t *map)
{
    return map ? map->bucket_count : 0;
}

void ecs_map_clear(
    ecs_map_t *map)
{
    ecs_assert(map != NULL, ECS_INVALID_PARAMETER, NULL);
    clear_buckets(map);
    map->count = 0;
    ensure_buckets(map, 2);
}

ecs_map_iter_t ecs_map_iter(
    const ecs_map_t *map)
{
    return (ecs_map_iter_t){
        .map = map,
        .bucket = NULL,
        .bucket_index = 0,
        .element_index = 0
    };
}

void* _ecs_map_next(
    ecs_map_iter_t *iter,
    ecs_size_t elem_size,
    ecs_map_key_t *key_out)
{
    const ecs_map_t *map = iter->map;
    if (!ecs_map_is_initialized(map)) {
        return NULL;
    }
    
    ecs_assert(!elem_size || elem_size == map->elem_size, 
        ECS_INVALID_PARAMETER, NULL);
 
    ecs_bucket_t *bucket = iter->bucket;
    int32_t element_index = iter->element_index;
    elem_size = map->elem_size;

    do {
        if (!bucket) {
            int32_t bucket_index = iter->bucket_index;
            ecs_bucket_t *buckets = map->buckets;
            if (bucket_index < map->bucket_count) {
                bucket = &buckets[bucket_index];
                iter->bucket = bucket;

                element_index = 0;
                iter->element_index = 0;
            } else {
                return NULL;
            }
        }

        if (element_index < bucket->count) {
            iter->element_index = element_index + 1;
            break;
        } else {
            bucket = NULL;
            iter->bucket_index ++;
        }
    } while (true);
    
    if (key_out) {
        *key_out = bucket->keys[element_index];
    }

    return GET_ELEM(bucket->payload, elem_size, element_index);
}

void* _ecs_map_next_ptr(
    ecs_map_iter_t *iter,
    ecs_map_key_t *key_out)
{
    void *result = _ecs_map_next(iter, ECS_SIZEOF(void*), key_out);
    if (result) {
        return *(void**)result;
    } else {
        return NULL;
    }
}

void ecs_map_grow(
    ecs_map_t *map, 
    int32_t element_count)
{
    ecs_assert(map != NULL, ECS_INVALID_PARAMETER, NULL);
    int32_t target_count = map->count + element_count;
    int32_t bucket_count = get_bucket_count(target_count);

    if (bucket_count > map->bucket_count) {
        rehash(map, bucket_count);
    }
}

void ecs_map_set_size(
    ecs_map_t *map, 
    int32_t element_count)
{    
    ecs_assert(map != NULL, ECS_INVALID_PARAMETER, NULL);
    int32_t bucket_count = get_bucket_count(element_count);

    if (bucket_count) {
        rehash(map, bucket_count);
    }
}

ecs_map_t* ecs_map_copy(
    ecs_map_t *map)
{
    if (!ecs_map_is_initialized(map)) {
        return NULL;
    }

    ecs_size_t elem_size = map->elem_size;
    ecs_map_t *result = _ecs_map_new(map->elem_size, ecs_map_count(map));

    ecs_map_iter_t it = ecs_map_iter(map);
    ecs_map_key_t key;
    void *ptr;
    while ((ptr = _ecs_map_next(&it, elem_size, &key))) {
        _ecs_map_set(result, elem_size, key, ptr);
    }

    return result;
}

void ecs_map_memory(
    ecs_map_t *map, 
    int32_t *allocd,
    int32_t *used)
{
    ecs_assert(map != NULL, ECS_INVALID_PARAMETER, NULL);

    if (used) {
        *used = map->count * map->elem_size;
    }

    if (allocd) {
        *allocd += ECS_SIZEOF(ecs_map_t);

        int i, bucket_count = map->bucket_count;
        for (i = 0; i < bucket_count; i ++) {
            ecs_bucket_t *bucket = &map->buckets[i];
            *allocd += KEY_SIZE * bucket->count;
            *allocd += map->elem_size * bucket->count;
        }

        *allocd += ECS_SIZEOF(ecs_bucket_t) * bucket_count;
    }
}
