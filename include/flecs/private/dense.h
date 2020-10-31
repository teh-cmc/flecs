#ifndef FLECS_DENSE_H
#define FLECS_DENSE_H

#include "api_defines.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    ecs_paged_t sparse;         /* Paged array with sparse payload */
    ecs_paged_t dense;
    // ecs_vector_t *dense;        /* Dense array with indices to sparse array. The
    //                              * dense array stores both alive and not alive
    //                              * sparse indices. The 'count' member keeps
    //                              * track of which indices are alive. */
    // ecs_vector_t *data;         /* Dense payload */
    int16_t size;
} ecs_dense_t;

FLECS_DBG_API void _ecs_dense_init(
    ecs_dense_t *dense,
    ecs_size_t size);

FLECS_DBG_API void ecs_dense_fini(
    ecs_dense_t *dense);

FLECS_DBG_API ecs_dense_t* _ecs_dense_new(
    ecs_size_t dense_size);

FLECS_DBG_API void ecs_dense_free(
    ecs_dense_t *dense);

FLECS_DBG_API void* _ecs_dense_get(
    const ecs_dense_t *dense,
    ecs_size_t size,
    uint64_t id);

FLECS_DBG_API bool _ecs_dense_ensure(
    ecs_dense_t *dense,
    ecs_size_t size,
    uint64_t id);

struct ecs_dense_ensure_get_t {
    void *ptr;
    bool added;
};

FLECS_DBG_API struct ecs_dense_ensure_get_t _ecs_dense_ensure_get(
    ecs_dense_t *dense,
    ecs_size_t size,
    uint64_t id); 

FLECS_DBG_API bool ecs_dense_remove(
    ecs_dense_t *dense,
    uint64_t id);

FLECS_DBG_API void ecs_dense_clear(
    ecs_dense_t *d);

FLECS_DBG_API int32_t ecs_dense_count(
    const ecs_dense_t *d);

FLECS_DBG_API int32_t ecs_dense_size(
    const ecs_dense_t *d);

#define ecs_dense_new(T)\
    _ecs_dense_new(sizeof(T))

#define ecs_dense_get(dense, T, id)\
    _ecs_dense_get(dense, sizeof(T), id)

#define ecs_dense_ensure(dense, T, id)\
    _ecs_dense_ensure(dense, sizeof(T), id)

#define ecs_dense_ensure_get(dense, T, id)\
    _ecs_dense_ensure_get(dense, sizeof(T), id)

#ifdef __cplusplus
}
#endif

#endif
