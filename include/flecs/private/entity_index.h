#ifndef FLECS_ENTITY_INDEX_H
#define FLECS_ENTITY_INDEX_H

#include "api_defines.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ecs_eis_get(world, entity) ecs_sparse_get(world->entity_index, ecs_record_t, entity)
#define ecs_eis_get_any(world, entity) ecs_sparse_get_any(world->entity_index, ecs_record_t, entity)
#define ecs_eis_ensure(world, entity) ecs_sparse_ensure(world->entity_index, ecs_record_t, entity)
#define ecs_eis_delete(world, entity) ecs_sparse_remove(world->entity_index, entity)
#define ecs_eis_set_generation(world, entity) ecs_sparse_set_generation(world->entity_index, entity)
#define ecs_eis_is_alive(world, entity) ecs_sparse_is_alive(world->entity_index, entity)
#define ecs_eis_exists(world, entity) ecs_sparse_exists(world->entity_index, entity)
#define ecs_eis_recycle(world) ecs_sparse_new_id(world->entity_index)
#define ecs_eis_set_size(world, size) ecs_sparse_set_size(world->entity_index, size)
#define ecs_eis_count(world) ecs_sparse_count(world->entity_index)
#define ecs_eis_clear(world) ecs_sparse_clear(world->entity_index)
#define ecs_eis_copy(world) ecs_sparse_copy(world->entity_index)
#define ecs_eis_free(world) ecs_sparse_free(world->entity_index)
#define ecs_eis_memory(world, allocd, used) ecs_sparse_memory(world->entity_index, allocd, used)

#ifdef __cplusplus
}
#endif

#endif
