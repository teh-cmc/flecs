#ifndef FLECS_VECTOR_H
#define FLECS_VECTOR_H

#include "api_defines.h"

typedef struct ecs_vector_t ecs_vector_t;

typedef int (*ecs_comparator_t)(
    const void* p1,
    const void *p2);

#ifdef __cplusplus
extern "C" {
#endif

FLECS_DBG_API ecs_vector_t* _ecs_vector_new(
    ecs_size_t elem_size,
    int32_t elem_count);

FLECS_DBG_API ecs_vector_t* _ecs_vector_from_array(
    ecs_size_t elem_size,
    int32_t elem_count,
    void *array);

FLECS_DBG_API void ecs_vector_free(
    ecs_vector_t *vector);

FLECS_DBG_API void ecs_vector_clear(
    ecs_vector_t *vector);

FLECS_DBG_API void ecs_vector_assert_size(
    ecs_vector_t* vector_inout,
    ecs_size_t elem_size);  

FLECS_DBG_API void* _ecs_vector_add(
    ecs_vector_t **array_inout,
    ecs_size_t elem_size);

FLECS_DBG_API void* _ecs_vector_addn(
    ecs_vector_t **array_inout,
    ecs_size_t elem_size,
    int32_t elem_count);

FLECS_DBG_API void* _ecs_vector_swap(
    ecs_vector_t **array_inout,
    ecs_size_t elem_size);

FLECS_DBG_API void* _ecs_vector_get(
    const ecs_vector_t *vector,
    ecs_size_t elem_size,
    int32_t index);

FLECS_DBG_API void* _ecs_vector_last(
    const ecs_vector_t *vector,
    ecs_size_t elem_size);

FLECS_DBG_API int32_t _ecs_vector_set_min_size(
    ecs_vector_t **array_inout,
    ecs_size_t elem_size,
    int32_t elem_count);

FLECS_DBG_API int32_t _ecs_vector_set_min_count(
    ecs_vector_t **vector_inout,
    ecs_size_t elem_size,
    int32_t elem_count);

FLECS_DBG_API void ecs_vector_remove_last(
    ecs_vector_t *vector);

FLECS_DBG_API bool _ecs_vector_pop(
    ecs_vector_t *vector,
    ecs_size_t elem_size,
    void *value);

FLECS_DBG_API int32_t _ecs_vector_remove(
    ecs_vector_t *vector,
    ecs_size_t elem_size,
    int32_t index);

FLECS_DBG_API void _ecs_vector_reclaim(
    ecs_vector_t **vector,
    ecs_size_t elem_size);

FLECS_DBG_API int32_t _ecs_vector_grow(
    ecs_vector_t **vector,
    ecs_size_t elem_size,
    int32_t elem_count);

FLECS_DBG_API int32_t _ecs_vector_set_size(
    ecs_vector_t **vector,
    ecs_size_t elem_size,
    int32_t elem_count);

FLECS_DBG_API int32_t _ecs_vector_set_count(
    ecs_vector_t **vector,
    ecs_size_t elem_size,
    int32_t elem_count);

FLECS_DBG_API int32_t ecs_vector_count(
    const ecs_vector_t *vector);

FLECS_DBG_API int32_t ecs_vector_size(
    const ecs_vector_t *vector);

FLECS_DBG_API void* _ecs_vector_first(
    const ecs_vector_t *vector,
    ecs_size_t elem_size);

FLECS_DBG_API void _ecs_vector_sort(
    ecs_vector_t *vector,
    ecs_size_t elem_size,
    ecs_comparator_t compare_action);

FLECS_DBG_API void _ecs_vector_memory(
    const ecs_vector_t *vector,
    ecs_size_t elem_size,
    int32_t *allocd,
    int32_t *used);

FLECS_DBG_API ecs_vector_t* _ecs_vector_copy(
    const ecs_vector_t *src,
    ecs_size_t elem_size);


#define ecs_vector_new(T, elem_count) \
    _ecs_vector_new(ECS_SIZEOF(T), elem_count) 

#define ecs_vector_from_array(T, elem_count, array)\
    _ecs_vector_from_array(ECS_SIZEOF(T), elem_count, array)
    
#define ecs_vector_add(vector, T) \
    ((T*)_ecs_vector_add(vector, ECS_SIZEOF(T)))

#define ecs_vector_addn(vector, T, elem_count) \
    ((T*)_ecs_vector_addn(vector, ECS_SIZEOF(T), elem_count))

#define ecs_vector_swap(vector, T, elem_count) \
    ((T*)_ecs_vector_swap(vector, ECS_SIZEOF(T), elem_count))

#define ecs_vector_swap_t(vector, size, alignment, elem_count) \
    _ecs_vector_swap(vector, ECS_VECTOR_U(size, alignment), elem_count)

#define ecs_vector_get(vector, T, index) \
    ((T*)_ecs_vector_get(vector, ECS_SIZEOF(T), index))

#define ecs_vector_last(vector, T) \
    (T*)_ecs_vector_last(vector, ECS_SIZEOF(T))

#define ecs_vector_set_min_size(vector, T, size) \
    _ecs_vector_set_min_size(vector, ECS_SIZEOF(T), size)

#define ecs_vector_set_min_count(vector, T, size) \
    _ecs_vector_set_min_count(vector, ECS_SIZEOF(T), size)

#define ecs_vector_pop(vector, T, value) \
    _ecs_vector_pop(vector, ECS_SIZEOF(T), value)

#define ecs_vector_remove(vector, T, index) \
    _ecs_vector_remove(vector, ECS_SIZEOF(T), index)

#define ecs_vector_reclaim(vector, T)\
    _ecs_vector_reclaim(vector, ECS_SIZEOF(T))

#define ecs_vector_grow(vector, T, size) \
    _ecs_vector_grow(vector, ECS_SIZEOF(T), size)

#define ecs_vector_set_size(vector, T, elem_count) \
    _ecs_vector_set_size(vector, ECS_SIZEOF(T), elem_count)

#define ecs_vector_set_count(vector, T, elem_count) \
    _ecs_vector_set_count(vector, ECS_SIZEOF(T), elem_count)
    
#define ecs_vector_first(vector, T) \
    ((T*)_ecs_vector_first(vector, ECS_SIZEOF(T)))
    
#define ecs_vector_sort(vector, T, compare_action) \
    _ecs_vector_sort(vector, ECS_SIZEOF(T), compare_action)
    
#define ecs_vector_memory(vector, T, allocd, used) \
    _ecs_vector_memory(vector, ECS_SIZEOF(T), allocd, used)

#define ecs_vector_copy(src, T) \
    _ecs_vector_copy(src, ECS_SIZEOF(T))

#ifndef FLECS_LEGACY
#define ecs_vector_each(vector, T, var, ...)\
    {\
        int var##_i, var##_count = ecs_vector_count(vector);\
        T* var##_array = ecs_vector_first(vector, T);\
        for (var##_i = 0; var##_i < var##_count; var##_i ++) {\
            T* var = &var##_array[var##_i];\
            __VA_ARGS__\
        }\
    }
#endif

#ifdef __cplusplus
}
#endif

#endif
