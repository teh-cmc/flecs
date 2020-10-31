#include "private_api.h"

static
ecs_store_policy_t* store_default_policy(
    ecs_world_t *world,
    ecs_entity_t id)
{
    ecs_assert(
        ecs_map_get(world->store_policies, ecs_store_policy_t, id) == NULL, 
        ECS_INTERNAL_ERROR);

    ecs_store_policy_t *sp = ecs_map_ensure(
        world->store_policies, ecs_store_policy_t, id);
    ecs_assert(sp != NULL, ECS_INTERNAL_ERROR);

    *sp = (ecs_store_policy_t){
        .sparse = false,
        .partition = 0
    };

    return sp;
}

static
ecs_store_policy_t* store_ensure_policy(
    ecs_world_t *world,
    ecs_entity_t id)
{
    /* First, try finding an exact match for the provided id */
    ecs_store_policy_t *sp = ecs_map_get(
        world->store_policies, ecs_store_policy_t, id);
    if (sp) {
        return sp;
    }

    /* If no exact match was found and the id is not a pair, no policy
     * is available yet. Create and return the default one. This ensures that
     * subsequent attempts to register a storage policy will fail. */
    if (!ecs_has_flag(id, ECS_PAIR)) {
        return store_default_policy(world, id);
    }

    /* If there is no exact match, try finding a wildcard policy if the provided
     * id is a pair. */

    /* First try finding a policy for the role */
    ecs_entity_t id_x = ecs_pair(ecs_get_role(id), EcsWildcard);
    sp = ecs_map_get(world->store_policies, ecs_store_policy_t, id_x);
    if (!sp) {
        /* Create the default policy for this wildcard pair. If left undefined,
         * an application could define a wildcard policy later that conflicts 
         * with the one returned for the provided pair. */
        store_default_policy(world, id_x);
    }

    /* If not found, try finding one for the subject */
    if (!sp) {
        ecs_entity_t x_id = ecs_pair(EcsWildcard, ecs_get_subject(id));
        sp = ecs_map_get(world->store_policies, ecs_store_policy_t, x_id);
        if (!sp) {
            /* Create default policy as per above */
            store_default_policy(world, x_id);
        }
    }

    /* If not found, try finding one for any kind of pair */
    if (!sp) {
        ecs_entity_t x_x = ecs_pair(EcsWildcard, EcsWildcard);
        sp = ecs_map_get(world->store_policies, ecs_store_policy_t, x_x);
        if (!sp) {
            /* Create default policy as per above */
            store_default_policy(world, x_x);
        }
    }

    /* If not found, return the default policy */
    if (!sp) {
        sp = store_default_policy(world, id);
    }

    return sp;
}

static
ecs_store_t* store_create(
    ecs_world_t *world,
    ecs_entity_t id)
{
    /* Storage id may not contain wildcards */
    ecs_assert(ecs_get_subject(id) != EcsWildcard, ECS_INVALID_PARAMETER);
    ecs_assert(ecs_get_role(id) != EcsWildcard, ECS_INVALID_PARAMETER);

    ecs_store_policy_t *sp = store_ensure_policy(world, id);
    ecs_assert(sp != NULL, ECS_INTERNAL_ERROR);

    ecs_store_t *store = ecs_ptree_ensure(&world->stores, ecs_store_t, id);
    ecs_assert(store != NULL, ECS_INTERNAL_ERROR);
    ecs_assert(!store->initialized, ECS_INVALID_PARAMETER);

    if (sp->sparse) {
        const EcsType *ptr = ecs_get_type_from_id(world, id);
        ecs_size_t size = ptr ? ptr->size : 0;
        store->sparse = _ecs_dense_new(size);
    }

    store->initialized = true;

    return store;
}

void ecs_set_storage(
    ecs_world_t *world,
    ecs_entity_t id,
    ecs_store_policy_t policy,
    bool is_hint)
{
    if (ecs_map_get(world->store_policies, ecs_store_policy_t, id)) {
        ecs_assert(is_hint, ECS_INVALID_PARAMETER);
        return;
    }

    ecs_store_policy_t *sp = ecs_map_ensure(
        world->store_policies, ecs_store_policy_t, id);
    ecs_assert(sp != NULL, ECS_INTERNAL_ERROR);

    *sp = policy;

    /* If this is a composite id with wildcards, don't create the store */
    if (ecs_has_flag(id, ECS_PAIR)) {
        if ((ecs_get_subject(id) == EcsWildcard) || 
            (ecs_get_role(id) == EcsWildcard))
        {
            return;
        }
    }

    store_create(world, id);
}

ecs_store_t* ecs_store_get(
    const ecs_world_t *world,
    ecs_entity_t id)
{
    ecs_store_t *store = ecs_ptree_get(&world->stores, ecs_store_t, id);
    if (!store || !store->initialized) {
        return NULL;
    }

    return store;
}

ecs_store_t* ecs_store_ensure(
    ecs_world_t *world,
    ecs_entity_t id)
{
    ecs_store_t *store = ecs_store_get(world, id);
    if (!store) {
        store = store_create(world, id);
    }

    ecs_assert(store->initialized, ECS_INTERNAL_ERROR);

    return store;
}
