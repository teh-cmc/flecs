#ifndef FLECS_PTREE_H
#define FLECS_PTREE_H

#include "api_defines.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct addr_t {
    uint16_t value[4];
} addr_t;

typedef struct {
    void *data;
    uint16_t offset;
    int32_t length;
} array_t;

typedef struct {
    array_t data;
    array_t pages;
} page_t;

typedef struct {
    page_t root;
    void *first_65k;
    uint16_t min_65k;
    uint16_t max_65k;
    uint8_t elem_size;
} ecs_ptree_t;

typedef struct {
    ecs_ptree_t *ptree;
    void *frames[3];
    uint16_t cur_page[3];
    int32_t cur_elem;
    int8_t sp;
    uint64_t index;
} ecs_ptree_iter_t;

FLECS_DBG_API void _ecs_ptree_init(
    ecs_ptree_t *ptree,
    ecs_size_t elem_size);

FLECS_DBG_API ecs_ptree_t* _ecs_ptree_new(
    ecs_size_t elem_size);

FLECS_DBG_API int32_t ecs_ptree_fini(
    ecs_ptree_t *ptree);

FLECS_DBG_API int32_t ecs_ptree_free(
    ecs_ptree_t *ptree);

FLECS_DBG_API void* _ecs_ptree_ensure(
    ecs_ptree_t *ptree,
    ecs_size_t elem_size,
    uint64_t index);

FLECS_DBG_API void* _ecs_ptree_get(
    const ecs_ptree_t *ptree,
    ecs_size_t elem_size,
    uint64_t index);

FLECS_DBG_API ecs_ptree_iter_t ecs_ptree_iter(
    ecs_ptree_t *ptree);

FLECS_DBG_API ecs_ptree_iter_t ecs_ptiny_iter(
    ecs_ptree_t *ptree);

FLECS_DBG_API void* _ecs_ptree_next(
    ecs_ptree_iter_t *it,
    ecs_size_t elem_size);    

FLECS_DBG_API void* _ecs_ptiny_next(
    ecs_ptree_iter_t *it,
    ecs_size_t elem_size);

#define ecs_ptree_init(ptree, T)\
    _ecs_ptree_init(ptree, ECS_SIZEOF(T))

#define ecs_ptree_new(T)\
    _ecs_ptree_new(ECS_SIZEOF(T))

#define ecs_ptree_get(ptree, T, index)\
    (T*)_ecs_ptree_get(ptree, ECS_SIZEOF(T), index)

#define ecs_ptree_ensure(ptree, T, index)\
    _ecs_ptree_ensure(ptree, ECS_SIZEOF(T), index)

#define ecs_ptree_next(iter, T)\
    _ecs_ptree_next(iter, ECS_SIZEOF(T))


/* -- Low footprint version, do not mix new/get/ensure API calls -- */

FLECS_DBG_API ecs_ptree_t* _ecs_ptiny_new(
    ecs_size_t elem_size);

FLECS_DBG_API void _ecs_ptiny_init(
    ecs_ptree_t *ptree,
    ecs_size_t elem_size);

FLECS_DBG_API void* _ecs_ptiny_ensure(
    ecs_ptree_t *ptree,
    ecs_size_t elem_size,
    uint64_t index);

FLECS_DBG_API void* _ecs_ptiny_get(
    const ecs_ptree_t *ptree,
    ecs_size_t elem_size,
    uint64_t index);

#define ecs_ptiny_init(ptree, T)\
    _ecs_ptiny_init(ptree, ECS_SIZEOF(T))

#define ecs_ptiny_new(T)\
    _ecs_ptiny_new(ECS_SIZEOF(T))

#define ecs_ptiny_get(ptree, T, index)\
    (T*)_ecs_ptiny_get(ptree, ECS_SIZEOF(T), index)

#define ecs_ptiny_ensure(ptree, T, index)\
    _ecs_ptiny_ensure(ptree, ECS_SIZEOF(T), index)

#define ecs_ptiny_next(iter, T)\
    _ecs_ptiny_next(iter, ECS_SIZEOF(T))

#define ecs_ptiny_free(ptree)\
    ecs_ptree_free(ptree)

#ifdef __cplusplus
}
#endif

#endif
