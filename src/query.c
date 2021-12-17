#include "private_api.h"

#ifdef FLECS_SYSTEM
#include "addons/system/system.h"
#endif

static
void compute_group_id(
    ecs_query_t *query,
    ecs_query_table_match_t *match)
{
    ecs_assert(match != NULL, ECS_INTERNAL_ERROR, NULL);

    if (query->group_by) {
        ecs_table_t *table = match->table;
        ecs_assert(table != NULL, ECS_INTERNAL_ERROR, NULL);

        match->group_id = query->group_by(query->world, table->type, 
            query->group_by_id, query->group_by_ctx);
    } else {
        match->group_id = 0;
    }
}

static
ecs_query_table_list_t* get_group(
    ecs_query_t *query,
    uint64_t group_id)
{
    return ecs_map_get(query->groups, ecs_query_table_list_t, group_id);
}

static
ecs_query_table_list_t* ensure_group(
    ecs_query_t *query,
    uint64_t group_id)
{
    return ecs_map_ensure(query->groups, ecs_query_table_list_t, group_id);
}

/* Find the last node of the group after which this group should be inserted */
static
ecs_query_table_node_t* find_group_insertion_node(
    ecs_query_t *query,
    uint64_t group_id)
{
    /* Grouping must be enabled */
    ecs_assert(query->group_by != NULL, ECS_INTERNAL_ERROR, NULL);

    ecs_map_iter_t it = ecs_map_iter(query->groups);
    ecs_query_table_list_t *list, *closest_list = NULL;
    uint64_t id, closest_id = 0;

    /* Find closest smaller group id */
    while ((list = ecs_map_next(&it, ecs_query_table_list_t, &id))) {
        if (id >= group_id) {
            continue;
        }

        if (!list->last) {
            ecs_assert(list->first == NULL, ECS_INTERNAL_ERROR, NULL);
            continue;
        }

        if (!closest_list || ((group_id - id) < (group_id - closest_id))) {
            closest_id = id;
            closest_list = list;
        }
    }

    if (closest_list) {
        return closest_list->last;
    } else {
        return NULL; /* Group should be first in query */
    }
}

/* Initialize group with first node */
static
void create_group(
    ecs_query_t *query,
    ecs_query_table_node_t *node)
{
    ecs_query_table_match_t *match = node->match;
    uint64_t group_id = match->group_id;

    /* If query has grouping enabled & this is a new/empty group, find
     * the insertion point for the group */
    ecs_query_table_node_t *insert_after = find_group_insertion_node(
        query, group_id);

    if (!insert_after) {
        /* This group should appear first in the query list */
        ecs_query_table_node_t *query_first = query->list.first;
        if (query_first) {
            /* If this is not the first match for the query, insert before it */
            node->next = query_first;
            query_first->prev = node;
            query->list.first = node;
        } else {
            /* If this is the first match of the query, initialize its list */
            ecs_assert(query->list.last == NULL, ECS_INTERNAL_ERROR, NULL);
            query->list.first = node;
            query->list.last = node;
        }
    } else {
        ecs_assert(query->list.first != NULL, ECS_INTERNAL_ERROR, NULL);
        ecs_assert(query->list.last != NULL, ECS_INTERNAL_ERROR, NULL);

        /* This group should appear after another group */
        ecs_query_table_node_t *insert_before = insert_after->next;
        node->prev = insert_after;
        insert_after->next = node;
        node->next = insert_before;
        if (insert_before) {
            insert_before->prev = node;
        } else {
            ecs_assert(query->list.last == insert_after, 
                ECS_INTERNAL_ERROR, NULL);
                
            /* This group should appear last in the query list */
            query->list.last = node;
        }
    }
}

static
void remove_group(
    ecs_query_t *query,
    uint64_t group_id)
{
    ecs_map_remove(query->groups, group_id);
}

/* Find the list the node should be part of */
static
ecs_query_table_list_t* get_node_list(
    ecs_query_t *query,
    ecs_query_table_node_t *node)
{
    ecs_query_table_match_t *match = node->match;
    if (query->group_by) {
        return get_group(query, match->group_id);
    } else {
        return &query->list;
    }
}

/* Find or create the list the node should be part of */
static
ecs_query_table_list_t* ensure_node_list(
    ecs_query_t *query,
    ecs_query_table_node_t *node)
{
    ecs_query_table_match_t *match = node->match;
    if (query->group_by) {
        return ensure_group(query, match->group_id);
    } else {
        return &query->list;
    }
}

/* Remove node from list */
static
void remove_table_node(
    ecs_query_t *query,
    ecs_query_table_node_t *node)
{
    ecs_query_table_node_t *prev = node->prev;
    ecs_query_table_node_t *next = node->next;

    ecs_assert(prev != node, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(next != node, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(!prev || prev != next, ECS_INTERNAL_ERROR, NULL);

    ecs_query_table_list_t *list = get_node_list(query, node);

    if (!list || !list->first) {
        /* If list contains no nodes, the node must be empty */
        ecs_assert(!list || list->last == NULL, ECS_INTERNAL_ERROR, NULL);
        ecs_assert(prev == NULL, ECS_INTERNAL_ERROR, NULL);
        ecs_assert(next == NULL, ECS_INTERNAL_ERROR, NULL);
        return;
    }

    ecs_assert(prev != NULL || query->list.first == node, 
        ECS_INTERNAL_ERROR, NULL);
    ecs_assert(next != NULL || query->list.last == node, 
        ECS_INTERNAL_ERROR, NULL);

    if (prev) {
        prev->next = next;
    }
    if (next) {
        next->prev = prev;
    }

    ecs_assert(list->count > 0, ECS_INTERNAL_ERROR, NULL);
    list->count --;

    if (query->group_by) {
        ecs_query_table_match_t *match = node->match;
        uint64_t group_id = match->group_id;

        /* Make sure query.list is updated if this is the first or last group */
        if (query->list.first == node) {
            ecs_assert(prev == NULL, ECS_INTERNAL_ERROR, NULL);
            query->list.first = next;
            prev = next;
        }
        if (query->list.last == node) {
            ecs_assert(next == NULL, ECS_INTERNAL_ERROR, NULL);
            query->list.last = prev;
            next = prev;
        }

        ecs_assert(query->list.count > 0, ECS_INTERNAL_ERROR, NULL);
        query->list.count --;

        /* Make sure group list only contains nodes that belong to the group */
        if (prev && prev->match->group_id != group_id) {
            /* The previous node belonged to another group */
            prev = next;
        }
        if (next && next->match->group_id != group_id) {
            /* The next node belonged to another group */
            next = prev;
        }

        /* Do check again, in case both prev & next belonged to another group */
        if (prev && prev->match->group_id != group_id) {
            /* There are no more matches left in this group */
            remove_group(query, group_id);
            list = NULL;
        }
    }

    if (list) {
        if (list->first == node) {
            list->first = next;
        }
        if (list->last == node) {
            list->last = prev;
        }
    }

    node->prev = NULL;
    node->next = NULL;

#ifdef FLECS_SYSTEM
    if (query->list.first == NULL && query->system && !query->world->is_fini) {
        ecs_system_activate(query->world, query->system, false, NULL);
    }
#endif

    query->match_count ++;
}

/* Add node to list */
static
void insert_table_node(
    ecs_query_t *query,
    ecs_query_table_node_t *node)
{
    /* Node should not be part of an existing list */
    ecs_assert(node->prev == NULL && node->next == NULL, 
        ECS_INTERNAL_ERROR, NULL);

    /* If this is the first match, activate system */
#ifdef FLECS_SYSTEM
    if (!query->list.first && query->system) {
        ecs_system_activate(query->world, query->system, true, NULL);
    }
#endif

    compute_group_id(query, node->match);

    ecs_query_table_list_t *list = ensure_node_list(query, node);
    if (list->last) {
        ecs_assert(query->list.first != NULL, ECS_INTERNAL_ERROR, NULL);
        ecs_assert(query->list.last != NULL, ECS_INTERNAL_ERROR, NULL);
        ecs_assert(list->first != NULL, ECS_INTERNAL_ERROR, NULL);

        ecs_query_table_node_t *last = list->last;
        ecs_query_table_node_t *last_next = last->next;

        node->prev = last;
        node->next = last_next;
        last->next = node;

        if (last_next) {
            last_next->prev = node;
        }

        list->last = node;

        if (query->group_by) {
            /* Make sure to update query list if this is the last group */
            if (query->list.last == last) {
                query->list.last = node;
            }
        }
    } else {
        ecs_assert(list->first == NULL, ECS_INTERNAL_ERROR, NULL);

        list->first = node;
        list->last = node;

        if (query->group_by) {
            /* Initialize group with its first node */
            create_group(query, node);
        }
    }

    if (query->group_by) {
        query->list.count ++;
    }

    list->count ++;
    query->match_count ++;

    ecs_assert(node->prev != node, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(node->next != node, ECS_INTERNAL_ERROR, NULL);

    ecs_assert(list->first != NULL, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(list->last != NULL, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(list->last == node, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(query->list.first != NULL, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(query->list.last != NULL, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(query->list.first->prev == NULL, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(query->list.last->next == NULL, ECS_INTERNAL_ERROR, NULL);
}

static
ecs_query_table_match_t* cache_add(
    ecs_query_table_t *elem)
{
    ecs_query_table_match_t *result = ecs_os_calloc_t(ecs_query_table_match_t);
    ecs_query_table_node_t *node = &result->node;

    node->match = result;
    if (!elem->first) {
        elem->first = result;
        elem->last = result;
    } else {
        ecs_assert(elem->last != NULL, ECS_INTERNAL_ERROR, NULL);
        elem->last->next_match = result;
        elem->last = result;
    }

    return result;
}

typedef struct {
    ecs_table_t *table;
    int32_t *dirty_state;
    int32_t column;
} table_dirty_state_t;

static
void get_dirty_state(
    ecs_query_t *query,
    ecs_query_table_match_t *match,
    int32_t term,
    table_dirty_state_t *out)
{
    ecs_world_t *world = query->world;
    ecs_entity_t subject = match->subjects[term];
    int32_t column;

    if (!subject) {
        out->table = match->table;
        column = match->columns[term];
        if (column == -1) {
            column = 0;
        }
    } else {
        out->table = ecs_get_table(world, subject);
        column = -match->columns[term];
    }

    out->dirty_state = flecs_table_get_dirty_state(out->table);

    if (column) {
        out->column = ecs_table_type_to_storage_index(out->table, column - 1);
    } else {
        out->column = -1;
    }
}

/* Get match monitor. Monitors are used to keep track of whether components 
 * matched by the query in a table have changed. */
static
bool get_match_monitor(
    ecs_query_t *query,
    ecs_query_table_match_t *match)
{
    if (match->monitor) {
        return false;
    }

    int32_t *monitor = ecs_os_calloc_n(int32_t, query->filter.term_count + 1);

    /* Mark terms that don't need to be monitored. This saves time when reading
     * and/or updating the monitor. */
    const ecs_filter_t *f = &query->filter;
    int32_t i, t = -1, term_count = f->term_count_actual;
    table_dirty_state_t cur_dirty_state;

    for (i = 0; i < term_count; i ++) {
        if (t == f->terms[i].index) {
            if (monitor[t + 1] != -1) {
                continue;
            }
        }

        t = f->terms[i].index;
        monitor[t + 1] = -1;

        if (f->terms[i].inout != EcsIn && 
            f->terms[i].inout != EcsInOut &&
            f->terms[i].inout != EcsInOutDefault) {
            continue; /* If term isn't read, don't monitor */
        }

        int32_t column = match->columns[t];
        if (column == 0) {
            continue; /* Don't track terms that aren't matched */
        }

        get_dirty_state(query, match, t, &cur_dirty_state);
        if (cur_dirty_state.column == -1) {
            continue; /* Don't track terms that aren't stored */
        }

        monitor[t + 1] = 0;
    }

    match->monitor = monitor;

    query->flags |= EcsQueryHasMonitor;

    return true;
}

/* Synchronize match monitor with table dirty state */
static
void sync_match_monitor(
    ecs_query_t *query,
    ecs_query_table_match_t *match)
{
    ecs_assert(match != NULL, ECS_INTERNAL_ERROR, NULL);
    if (!match->monitor) {
        if (query->flags & EcsQueryHasMonitor) {
            get_match_monitor(query, match);
        } else {
            return;
        }
    }

    int32_t *monitor = match->monitor;
    ecs_table_t *table = match->table;
    int32_t *dirty_state = flecs_table_get_dirty_state(table);
    ecs_assert(dirty_state != NULL, ECS_INTERNAL_ERROR, NULL);
    table_dirty_state_t cur;

    monitor[0] = dirty_state[0]; /* Did table gain/lose entities */

    int32_t i, term_count = query->filter.term_count_actual;
    for (i = 0; i < term_count; i ++) {
        int32_t t = query->filter.terms[i].index;
        if (monitor[t + 1] == -1) {
            continue;
        }
                
        get_dirty_state(query, match, t, &cur);
        ecs_assert(cur.column != -1, ECS_INTERNAL_ERROR, NULL);
        monitor[t + 1] = cur.dirty_state[cur.column + 1];
    }
}

/* Check if single match term has changed */
static
bool check_match_monitor_term(
    ecs_query_t *query,
    ecs_query_table_match_t *match,
    int32_t term)
{
    ecs_assert(match != NULL, ECS_INTERNAL_ERROR, NULL);

    if (get_match_monitor(query, match)) {
        return true;
    }
    
    int32_t *monitor = match->monitor;
    ecs_table_t *table = match->table;
    int32_t *dirty_state = flecs_table_get_dirty_state(table);
    ecs_assert(dirty_state != NULL, ECS_INTERNAL_ERROR, NULL);
    table_dirty_state_t cur;

    int32_t state = monitor[term];
    if (state == -1) {
        return false;
    }

    if (!term) {
        return monitor[0] != dirty_state[0];
    }

    get_dirty_state(query, match, term - 1, &cur);
    ecs_assert(cur.column != -1, ECS_INTERNAL_ERROR, NULL);

    return monitor[term] != cur.dirty_state[cur.column + 1];
}

/* Check if any term for match has changed */
static
bool check_match_monitor(
    ecs_query_t *query,
    ecs_query_table_match_t *match)
{
    ecs_assert(match != NULL, ECS_INTERNAL_ERROR, NULL);

    if (get_match_monitor(query, match)) {
        return true;
    }

    int32_t *monitor = match->monitor;
    ecs_table_t *table = match->table;
    int32_t *dirty_state = flecs_table_get_dirty_state(table);
    ecs_assert(dirty_state != NULL, ECS_INTERNAL_ERROR, NULL);
    table_dirty_state_t cur;

    if (monitor[0] != dirty_state[0]) {
        return true;
    }

    ecs_filter_t *f = &query->filter;
    int32_t i, term_count = f->term_count_actual;
    for (i = 0; i < term_count; i ++) {
        ecs_term_t *term = &f->terms[i];
        int32_t t = term->index;
        if (monitor[t + 1] == -1) {
            continue;
        }

        get_dirty_state(query, match, t, &cur);
        ecs_assert(cur.column != -1, ECS_INTERNAL_ERROR, NULL);

        if (monitor[t + 1] != cur.dirty_state[cur.column + 1]) {
            return true;
        }
    }

    return false;
}

/* Check if any term for matched table has changed */
static
bool check_table_monitor(
    ecs_query_t *query,
    ecs_query_table_t *table,
    int32_t term)
{
    ecs_query_table_node_t *cur, *end = table->last->node.next;

    for (cur = &table->first->node; cur != end; cur = cur->next) {
        ecs_query_table_match_t *match = (ecs_query_table_match_t*)cur;
        if (term == -1) {
            if (check_match_monitor(query, match)) {
                return true;
            }
        } else {
            if (check_match_monitor_term(query, match, term)) {
                return true;
            } 
        }
    }

    return false;
}

static
bool check_query_monitor(
    ecs_query_t *query)
{
    ecs_vector_t *vec = query->cache.tables;
    ecs_query_table_t *tables = ecs_vector_first(vec, ecs_query_table_t);
    int32_t i, count = ecs_vector_count(vec);

    for (i = 0; i < count; i ++) {
        ecs_query_table_t *qt = &tables[i];
        if (check_table_monitor(query, qt, -1)) {
            return true;
        }
    }

    return false;
}

static
void init_query_monitors(
    ecs_query_t *query)
{
    ecs_query_table_node_t *cur = query->list.first;

    /* Ensure each match has a monitor */
    for (; cur != NULL; cur = cur->next) {
        ecs_query_table_match_t *match = (ecs_query_table_match_t*)cur;
        get_match_monitor(query, match);
    }
}

/* Builtin group_by callback for Cascade terms.
 * This function traces the hierarchy depth of an entity type by following a
 * relation upwards (to its 'parents') for as long as those parents have the
 * specified component id. 
 * The result of the function is the number of parents with the provided 
 * component for a given relation. */
static
uint64_t group_by_cascade(
    ecs_world_t *world,
    ecs_type_t type,
    ecs_entity_t component,
    void *ctx)
{
    uint64_t result = 0;
    int32_t i, count = ecs_vector_count(type);
    ecs_entity_t *array = ecs_vector_first(type, ecs_entity_t);
    ecs_term_t *term = ctx;
    ecs_entity_t relation = term->subj.set.relation;

    /* Cascade needs a relation to calculate depth from */
    ecs_check(relation != 0, ECS_INVALID_PARAMETER, NULL);

    /* Should only be used with cascade terms */
    ecs_check(term->subj.set.mask & EcsCascade, ECS_INVALID_PARAMETER, NULL);

    /* Iterate back to front as relations are more likely to occur near the
     * end of a type. */
    for (i = count - 1; i >= 0; i --) {
        /* Find relation & relation object in entity type */
        if (ECS_HAS_RELATION(array[i], relation)) {
            ecs_type_t obj_type = ecs_get_type(world,     
                ecs_pair_object(world, array[i]));
            int32_t j, c_count = ecs_vector_count(obj_type);
            ecs_entity_t *c_array = ecs_vector_first(obj_type, ecs_entity_t);

            /* Iterate object type, check if it has the specified component */
            for (j = 0; j < c_count; j ++) {
                /* If it has the component, it is part of the tree matched by
                 * the query, increase depth */
                if (c_array[j] == component) {
                    result ++;

                    /* Recurse to test if the object has matching parents */
                    result += group_by_cascade(world, obj_type, component, ctx);
                    break;
                }
            }

            if (j != c_count) {
                break;
            }

        /* If the id doesn't have a role set, we'll find no more relations */
        } else if (!(array[i] & ECS_ROLE_MASK)) {
            break;
        }
    }

    return result;
error:
    return 0;
}

static
int32_t qsort_partition(
    ecs_world_t *world,
    ecs_table_t *table,
    ecs_data_t *data,
    ecs_entity_t *entities,
    void *ptr,    
    int32_t elem_size,
    int32_t lo,
    int32_t hi,
    ecs_order_by_action_t compare)
{
    int32_t p = (hi + lo) / 2;
    void *pivot = ECS_ELEM(ptr, elem_size, p);
    ecs_entity_t pivot_e = entities[p];
    int32_t i = lo - 1, j = hi + 1;
    void *el;    

repeat:
    {
        do {
            i ++;
            el = ECS_ELEM(ptr, elem_size, i);
        } while ( compare(entities[i], el, pivot_e, pivot) < 0);

        do {
            j --;
            el = ECS_ELEM(ptr, elem_size, j);
        } while ( compare(entities[j], el, pivot_e, pivot) > 0);

        if (i >= j) {
            return j;
        }

        flecs_table_swap(world, table, data, i, j);

        if (p == i) {
            pivot = ECS_ELEM(ptr, elem_size, j);
            pivot_e = entities[j];
        } else if (p == j) {
            pivot = ECS_ELEM(ptr, elem_size, i);
            pivot_e = entities[i];
        }

        goto repeat;
    }
}

static
void qsort_array(
    ecs_world_t *world,
    ecs_table_t *table,
    ecs_data_t *data,
    ecs_entity_t *entities,
    void *ptr,
    int32_t size,
    int32_t lo,
    int32_t hi,
    ecs_order_by_action_t compare)
{   
    if ((hi - lo) < 1)  {
        return;
    }

    int32_t p = qsort_partition(
        world, table, data, entities, ptr, size, lo, hi, compare);

    qsort_array(world, table, data, entities, ptr, size, lo, p, compare);

    qsort_array(world, table, data, entities, ptr, size, p + 1, hi, compare); 
}

static
void sort_table(
    ecs_world_t *world,
    ecs_table_t *table,
    int32_t column_index,
    ecs_order_by_action_t compare)
{
    ecs_data_t *data = &table->storage;
    if (!data->entities) {
        /* Nothing to sort */
        return;
    }

    int32_t count = flecs_table_data_count(data);
    if (count < 2) {
        return;
    }

    ecs_entity_t *entities = ecs_vector_first(data->entities, ecs_entity_t);

    void *ptr = NULL;
    int32_t size = 0;
    if (column_index != -1) {
        ecs_column_t *column = &data->columns[column_index];
        size = column->size;
        ptr = ecs_vector_first_t(column->data, size, column->alignment);
    }

    qsort_array(world, table, data, entities, ptr, size, 0, count - 1, compare);
}

/* Helper struct for building sorted table ranges */
typedef struct sort_helper_t {
    ecs_query_table_match_t *match;
    ecs_entity_t *entities;
    const void *ptr;
    int32_t row;
    int32_t elem_size;
    int32_t count;
    bool shared;
} sort_helper_t;

static
const void* ptr_from_helper(
    sort_helper_t *helper)
{
    ecs_assert(helper->row < helper->count, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(helper->elem_size >= 0, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(helper->row >= 0, ECS_INTERNAL_ERROR, NULL);
    if (helper->shared) {
        return helper->ptr;
    } else {
        return ECS_ELEM(helper->ptr, helper->elem_size, helper->row);
    }
}

static
ecs_entity_t e_from_helper(
    sort_helper_t *helper)
{
    if (helper->row < helper->count) {
        return helper->entities[helper->row];
    } else {
        return 0;
    }
}

static
void build_sorted_table_range(
    ecs_query_t *query,
    ecs_query_table_list_t *list)
{
    ecs_world_t *world = query->world;
    ecs_entity_t component = query->order_by_component;
    ecs_order_by_action_t compare = query->order_by;
    
    if (!list->count) {
        return;
    }

    int to_sort = 0;
    sort_helper_t *helper = ecs_os_malloc_n(sort_helper_t, list->count);
    ecs_query_table_node_t *cur, *end = list->last->next;
    for (cur = list->first; cur != end; cur = cur->next) {
        ecs_query_table_match_t *match = cur->match;
        ecs_table_t *table = match->table;
        ecs_data_t *data = &table->storage;
        ecs_vector_t *entities;
        if (!(entities = data->entities) || !ecs_table_count(table)) {
            continue;
        }

        int32_t index = ecs_type_index_of(table->storage_type, 0, component);
        if (index != -1) {
            ecs_column_t *column = &data->columns[index];
            int16_t size = column->size;
            int16_t align = column->alignment;
            helper[to_sort].ptr = ecs_vector_first_t(column->data, size, align);
            helper[to_sort].elem_size = size;
            helper[to_sort].shared = false;
        } else if (component) {
            /* Find component in prefab */
            ecs_entity_t base;
            ecs_type_match(world, table, table->type, 0, component, 
                EcsIsA, 1, 0, &base, NULL, NULL);

            /* If a base was not found, the query should not have allowed using
             * the component for sorting */
            ecs_assert(base != 0, ECS_INTERNAL_ERROR, NULL);

            const EcsComponent *cptr = ecs_get(world, component, EcsComponent);
            ecs_assert(cptr != NULL, ECS_INTERNAL_ERROR, NULL);

            helper[to_sort].ptr = ecs_get_id(world, base, component);
            helper[to_sort].elem_size = cptr->size;
            helper[to_sort].shared = true;
        } else {
            helper[to_sort].ptr = NULL;
            helper[to_sort].elem_size = 0;
            helper[to_sort].shared = false;
        }

        helper[to_sort].match = match;
        helper[to_sort].entities = ecs_vector_first(entities, ecs_entity_t);
        helper[to_sort].row = 0;
        helper[to_sort].count = ecs_table_count(table);
        to_sort ++;      
    }

    bool proceed;
    do {
        int32_t j, min = 0;
        proceed = true;

        ecs_entity_t e1;
        while (!(e1 = e_from_helper(&helper[min]))) {
            min ++;
            if (min == to_sort) {
                proceed = false;
                break;
            }
        }

        if (!proceed) {
            break;
        }

        for (j = min + 1; j < to_sort; j++) {
            ecs_entity_t e2 = e_from_helper(&helper[j]);
            if (!e2) {
                continue;
            }

            const void *ptr1 = ptr_from_helper(&helper[min]);
            const void *ptr2 = ptr_from_helper(&helper[j]);

            if (compare(e1, ptr1, e2, ptr2) > 0) {
                min = j;
                e1 = e_from_helper(&helper[min]);
            }
        }

        sort_helper_t *cur_helper = &helper[min];
        if (!cur || cur->match != cur_helper->match) {
            cur = ecs_vector_add(&query->table_slices, ecs_query_table_node_t);
            ecs_assert(cur != NULL, ECS_INTERNAL_ERROR, NULL);
            cur->match = cur_helper->match;
            cur->offset = cur_helper->row;
            cur->count = 1;
        } else {
            cur->count ++;
        }

        cur_helper->row ++;
    } while (proceed);

    /* Iterate through the vector of slices to set the prev/next ptrs. This
     * can't be done while building the vector, as reallocs may occur */
    int32_t i, count = ecs_vector_count(query->table_slices);    
    ecs_query_table_node_t *nodes = ecs_vector_first(
        query->table_slices, ecs_query_table_node_t);
    for (i = 0; i < count; i ++) {
        nodes[i].prev = &nodes[i - 1];
        nodes[i].next = &nodes[i + 1];
    }

    nodes[0].prev = NULL;
    nodes[i - 1].next = NULL;

    ecs_os_free(helper);
}

static
void build_sorted_tables(
    ecs_query_t *query)
{
    ecs_vector_clear(query->table_slices);

    if (query->group_by) {
        /* Populate sorted node list in grouping order */
        ecs_query_table_node_t *cur = query->list.first;
        if (cur) {
            do {
                /* Find list for current group */
                ecs_query_table_match_t *match = cur->match;
                ecs_assert(match != NULL, ECS_INTERNAL_ERROR, NULL);
                uint64_t group_id = match->group_id;
                ecs_query_table_list_t *list = ecs_map_get(query->groups, 
                    ecs_query_table_list_t, group_id);
                ecs_assert(list != NULL, ECS_INTERNAL_ERROR, NULL);

                /* Sort tables in current group */
                build_sorted_table_range(query, list);
                
                /* Find next group to sort */
                cur = list->last->next;
            } while (cur);
        }
    } else {
        build_sorted_table_range(query, &query->list);
    }
}

static
void sort_tables(
    ecs_world_t *world,
    ecs_query_t *query)
{
    ecs_order_by_action_t compare = query->order_by;
    if (!compare) {
        return;
    }
    
    ecs_entity_t order_by_component = query->order_by_component;
    int32_t i, order_by_term = -1;

    /* Find term that iterates over component (must be at least one) */
    if (order_by_component) {
        const ecs_filter_t *f = &query->filter;
        int32_t term_count = f->term_count_actual;
        for (i = 0; i < term_count; i ++) {
            ecs_term_t *term = &f->terms[i];
            if (term->subj.entity != EcsThis) {
                continue;
            }

            if (term->id == order_by_component) {
                order_by_term = i;
                break;
            }
        }

        ecs_assert(order_by_term != -1, ECS_INTERNAL_ERROR, NULL);
    }

    /* Iterate over non-empty tables. Don't bother with empty tables as they
     * have nothing to sort */

    bool tables_sorted = false;
    ecs_vector_t *vec = query->cache.tables;
    ecs_query_table_t *tables = ecs_vector_first(vec, ecs_query_table_t);
    int32_t count = ecs_vector_count(vec);

    for (i = 0; i < count; i ++) {
        ecs_query_table_t *qt = &tables[i];
        ecs_table_t *table = qt->hdr.table;
        bool dirty = false;

        if (check_table_monitor(query, qt, 0)) {
            dirty = true;
        }

        int32_t column = -1;
        if (order_by_component) {
            if (check_table_monitor(query, qt, order_by_term + 1)) {
                dirty = true;
            }

            if (dirty) {
                column = ecs_type_match(world, table->storage_table, 
                    table->storage_type, 0, order_by_component, 
                        0, 0, 0, 0, 0, 0);

                if (column == -1) {
                    /* Component is shared, no sorting is needed */
                    dirty = false;
                }
            }
        }

        if (!dirty) {
            continue;
        }

        /* Something has changed, sort the table */
        sort_table(world, table, column, compare);
        tables_sorted = true;
    }

    if (tables_sorted || query->match_count != query->prev_match_count) {
        build_sorted_tables(query);
        query->match_count ++; /* Increase version if tables changed */
    }
}

static
bool has_refs(
    ecs_query_t *query)
{
    ecs_term_t *terms = query->filter.terms;
    int32_t i, count = query->filter.term_count;

    for (i = 0; i < count; i ++) {
        ecs_term_t *term = &terms[i];
        ecs_term_id_t *subj = &term->subj;

        if (term->oper == EcsNot && !subj->entity) {
            /* Special case: if oper kind is Not and the query contained a
             * shared expression, the expression is translated to FromEmpty to
             * prevent resolving the ref */
            return true;
        } else if (subj->entity && (subj->entity != EcsThis || subj->set.mask != EcsSelf)) {
            /* If entity is not this, or if it can be substituted by other
             * entities, the query can have references. */
            return true;
        } 
    }

    return false;
}

static
bool has_pairs(
    ecs_query_t *query)
{
    ecs_term_t *terms = query->filter.terms;
    int32_t i, count = query->filter.term_count;

    for (i = 0; i < count; i ++) {
        if (ecs_id_is_wildcard(terms[i].id)) {
            return true;
        }
    }

    return false;    
}

static
void register_monitors(
    ecs_world_t *world,
    ecs_query_t *query)
{
    ecs_term_t *terms = query->filter.terms;
    int32_t i, count = query->filter.term_count;

    for (i = 0; i < count; i++) {
        ecs_term_t *term = &terms[i];
        ecs_term_id_t *subj = &term->subj;

        /* If component is requested with EcsCascade register component as a
         * parent monitor. Parent monitors keep track of whether an entity moved
         * in the hierarchy, which potentially requires the query to reorder its
         * tables. 
         * Also register a regular component monitor for EcsCascade columns.
         * This ensures that when the component used in the EcsCascade column
         * is added or removed tables are updated accordingly*/
        if (subj->set.mask & EcsSuperSet && subj->set.mask & EcsCascade && 
            subj->set.relation != EcsIsA) 
        {
            if (term->oper != EcsOr) {
                if (term->subj.set.relation != EcsIsA) {
                    flecs_monitor_register(
                        world, term->subj.set.relation, term->id, query);
                }
                flecs_monitor_register(world, 0, term->id, query);
            }

        /* FromAny also requires registering a monitor, as FromAny columns can
         * be matched with prefabs. The only term kinds that do not require
         * registering a monitor are FromOwned and FromEmpty. */
        } else if ((subj->set.mask & EcsSuperSet) || (subj->entity != EcsThis)){
            if (term->oper != EcsOr) {
                flecs_monitor_register(world, 0, term->id, query);
            }
        }
    };
}

static
void process_signature(
    ecs_world_t *world,
    ecs_query_t *query)
{
    ecs_term_t *terms = query->filter.terms;
    int32_t i, count = query->filter.term_count;

    for (i = 0; i < count; i ++) {
        ecs_term_t *term = &terms[i];
        ecs_term_id_t *pred = &term->pred;
        ecs_term_id_t *subj = &term->subj;
        ecs_term_id_t *obj = &term->obj;
        ecs_oper_kind_t op = term->oper; 
        ecs_inout_kind_t inout = term->inout;

        (void)pred;
        (void)obj;

        /* Queries do not support variables */
        ecs_check(pred->var != EcsVarIsVariable, 
            ECS_UNSUPPORTED, NULL);
        ecs_check(subj->entity == EcsThis || subj->var != EcsVarIsVariable, 
            ECS_UNSUPPORTED, NULL);
        ecs_check(obj->var != EcsVarIsVariable, 
            ECS_UNSUPPORTED, NULL);

        /* If self is not included in set, always start from depth 1 */
        if (!subj->set.min_depth && !(subj->set.mask & EcsSelf)) {
            subj->set.min_depth = 1;
        }

        if (inout != EcsIn) {
            query->flags |= EcsQueryHasOutColumns;
        }

        if (op == EcsOptional) {
            query->flags |= EcsQueryHasOptional;
        }

        if (!(query->flags & EcsQueryMatchDisabled)) {
            if (op == EcsAnd || op == EcsOr || op == EcsOptional) {
                if (term->id == EcsDisabled) {
                    query->flags |= EcsQueryMatchDisabled;
                }
            }
        }

        if (!(query->flags & EcsQueryMatchPrefab)) {
            if (op == EcsAnd || op == EcsOr || op == EcsOptional) {
                if (term->id == EcsPrefab) {
                    query->flags |= EcsQueryMatchPrefab;
                }
            }
        }

        if (subj->entity == EcsThis) {
            query->flags |= EcsQueryNeedsTables;
        }

        if (subj->set.mask & EcsCascade && term->oper == EcsOptional) {
            /* Query can only have one cascade column */
            ecs_assert(query->cascade_by == 0, ECS_INVALID_PARAMETER, NULL);
            query->cascade_by = i + 1;
        }

        if (subj->entity && subj->entity != EcsThis && 
            subj->set.mask == EcsSelf) 
        {
            flecs_add_flag(world, term->subj.entity, ECS_FLAG_OBSERVED);
        }
    }

    query->flags |= (ecs_flags32_t)(has_refs(query) * EcsQueryHasRefs);
    query->flags |= (ecs_flags32_t)(has_pairs(query) * EcsQueryHasTraits);

    if (!(query->flags & EcsQueryIsSubquery)) {
        register_monitors(world, query);
    }
error:
    return;
}

static
void add_table_match(
    ecs_world_t *world,
    ecs_query_t *query,
    ecs_table_t *table,
    bool has_shared,
    ecs_id_t *ids,
    int32_t *columns,
    ecs_entity_t *subjects,
    ecs_size_t *sizes)
{
    ecs_table_cache_t *qc = &query->cache;
    ecs_query_table_t *qt = ecs_table_cache_get(qc, ecs_query_table_t, table);
    if (!qt) {
        qt = ecs_table_cache_insert(qc, ecs_query_table_t, table);
    }

    ecs_query_table_match_t *table_data = cache_add(qt);
    table_data->table = table;

    int32_t t, term_count = query->filter.term_count;
    table_data->ids = ecs_os_memdup_n(ids, int32_t, term_count);
    table_data->columns = ecs_os_memdup_n(columns, int32_t, term_count);
    table_data->subjects = ecs_os_memdup_n(subjects, ecs_entity_t, term_count);
    table_data->sizes = ecs_os_memdup_n(sizes, ecs_size_t, term_count);

    /* Create refs for shared terms */
    if (has_shared) {
        int32_t r = 0, shared_count = 0;
        for (t = 0; t < term_count; t ++) {
            shared_count += subjects[t] != 0;
        }

        ecs_assert(shared_count != 0, ECS_INTERNAL_ERROR, NULL);
        table_data->references = ecs_os_malloc_n(ecs_ref_t, shared_count);

        for (t = 0; t < term_count; t ++) {
            ecs_entity_t src = subjects[t];
            if (src) {
                ecs_get_ref_id(world, &table_data->references[r ++], src, ids[t]);
                table_data->columns[t] = -r;
            }
        }
    }

    insert_table_node(query, &table_data->node);
}

static
bool match_table(
    ecs_world_t *world,
    ecs_query_t *query,
    ecs_table_t *table)
{
    ecs_filter_t *f = &query->filter;
    ecs_iter_t it = {.term_count = f->term_count};
    flecs_iter_init(&it);

    bool first = true;
    int32_t matches_left = 0;
    while (flecs_filter_match_table(world, f, table, 
        it.ids, 
        it.columns, 
        it.subjects,
        it.match_indices,
        &matches_left, first, -1))
    {
        add_table_match(world, query, table, 
            it.has_shared, it.ids, it.columns, it.subjects, it.sizes);

        first = false;

        if (!matches_left--) {
            break;
        }
    }

    ecs_iter_fini(&it);

    return false;
}

static
bool match_tables(
    ecs_world_t *world,
    ecs_query_t *query)
{
    ecs_iter_t it = ecs_filter_iter(world, &query->filter);
    while (ecs_filter_next(&it)) {
        add_table_match(world, query, it.table, 
            it.has_shared,
            it.ids, 
            it.columns, 
            it.subjects, 
            it.sizes);
    }

    return false;
}

/** When a table becomes empty remove it from the query list, or vice versa. */
static
void update_table(
    ecs_query_t *query,
    ecs_table_t *table,
    bool empty)
{
    int32_t prev_count = ecs_query_table_count(query);
    ecs_table_cache_set_empty(&query->cache, table, empty);
    int32_t cur_count = ecs_query_table_count(query);

    if (prev_count != cur_count) {
        ecs_query_table_t *qt = ecs_table_cache_get(
            &query->cache, ecs_query_table_t, table);
        ecs_assert(qt != NULL, ECS_INTERNAL_ERROR, NULL);
        ecs_query_table_match_t *cur, *next;

        for (cur = qt->first; cur != NULL; cur = next) {
            next = cur->next_match;

            if (empty) {
                ecs_assert(ecs_table_count(table) == 0, 
                    ECS_INTERNAL_ERROR, NULL);

                remove_table_node(query, &cur->node);
            } else {
                ecs_assert(ecs_table_count(table) != 0, 
                    ECS_INTERNAL_ERROR, NULL);
                insert_table_node(query, &cur->node);
            }
        }
    }

    ecs_assert(cur_count || query->list.first == NULL, 
        ECS_INTERNAL_ERROR, NULL);
}

static
void add_subquery(
    ecs_world_t *world, 
    ecs_query_t *parent, 
    ecs_query_t *subquery) 
{
    ecs_query_t **elem = ecs_vector_add(&parent->subqueries, ecs_query_t*);
    *elem = subquery;

    ecs_table_cache_t *cache = &parent->cache;
    ecs_vector_t *tables = cache->tables, *empty = cache->empty_tables;

    ecs_query_table_t *elems = ecs_vector_first(tables, ecs_query_table_t);
    int32_t i, count = ecs_vector_count(tables);
    for (i = 0; i < count; i ++) {
        match_table(world, subquery, elems[i].hdr.table);
    }

    elems = ecs_vector_first(empty, ecs_query_table_t);
    count = ecs_vector_count(empty);
    for (i = 0; i < count; i ++) {
        match_table(world, subquery, elems[i].hdr.table);
    }
}

static
void notify_subqueries(
    ecs_world_t *world,
    ecs_query_t *query,
    ecs_query_event_t *event)
{
    if (query->subqueries) {
        ecs_query_t **queries = ecs_vector_first(query->subqueries, ecs_query_t*);
        int32_t i, count = ecs_vector_count(query->subqueries);

        ecs_query_event_t sub_event = *event;
        sub_event.parent_query = query;

        for (i = 0; i < count; i ++) {
            ecs_query_t *sub = queries[i];
            flecs_query_notify(world, sub, &sub_event);
        }
    }
}

static
void resolve_cascade_subject_for_table(
    ecs_world_t *world,
    ecs_query_t *query,
    const ecs_table_t *table,
    ecs_type_t table_type,
    ecs_query_table_match_t *table_data)
{
    int32_t term_index = query->cascade_by - 1;
    ecs_term_t *term = &query->filter.terms[term_index];

    ecs_assert(table_data->references != 0, ECS_INTERNAL_ERROR, NULL);

    /* Obtain reference index */
    int32_t *column_indices = table_data->columns;
    int32_t ref_index = -column_indices[term_index] - 1;

    /* Obtain pointer to the reference data */
    ecs_ref_t *references = table_data->references;

    /* Find source for component */
    ecs_entity_t subject;
    ecs_type_match(world, table, table_type, 0, term->id, 
        term->subj.set.relation, 1, 0, &subject, NULL, NULL);

    /* If container was found, update the reference */
    if (subject) {
        ecs_ref_t *ref = &references[ref_index];
        ecs_assert(ref->component == term->id, ECS_INTERNAL_ERROR, NULL);

        references[ref_index].entity = ecs_get_alive(world, subject);
        table_data->subjects[term_index] = subject;
        ecs_get_ref_id(world, ref, subject, term->id);
    } else {
        references[ref_index].entity = 0;
        table_data->subjects[term_index] = 0;
    }

    if (ecs_table_count(table)) {
        /* The subject (or depth of the subject) may have changed, so reinsert
         * the node to make sure it's in the right group */
        remove_table_node(query, &table_data->node);
        insert_table_node(query, &table_data->node);
    }
}

static
void resolve_cascade_subject(
    ecs_world_t *world,
    ecs_query_t *query,
    ecs_query_table_t *elem,
    const ecs_table_t *table,
    ecs_type_t table_type)
{
    ecs_query_table_match_t *cur;
    for (cur = elem->first; cur != NULL; cur = cur->next_match) {
        resolve_cascade_subject_for_table(
            world, query, table, table_type, cur);
    }
}

/* Remove table */
static
void remove_table(
    ecs_poly_t *poly,
    void *ptr)
{
    ecs_poly_assert(poly, ecs_query_t);

    ecs_query_t *query = poly;
    ecs_query_table_t *elem = ptr;
    ecs_query_table_match_t *cur, *next;

    for (cur = elem->first; cur != NULL; cur = next) {
        ecs_os_free(cur->columns);
        ecs_os_free(cur->ids);
        ecs_os_free(cur->subjects);
        ecs_os_free(cur->sizes);
        ecs_os_free(cur->references);
        ecs_os_free(cur->sparse_columns);
        ecs_os_free(cur->bitset_columns);
        ecs_os_free(cur->monitor);

        if (!elem->hdr.empty) {
            remove_table_node(query, &cur->node);
        }

        next = cur->next_match;

        ecs_os_free(cur);
    }

    elem->first = NULL;
}

static
void unmatch_table(
    ecs_query_t *query,
    ecs_table_t *table)
{
    ecs_table_cache_remove(&query->cache, ecs_query_table_t, table);
}

static
void rematch_table(
    ecs_world_t *world,
    ecs_query_t *query,
    ecs_table_t *table)
{
    ecs_query_table_t *match = ecs_table_cache_get(
        &query->cache, ecs_query_table_t, table);

    if (match_table(world, query, table)) {
        /* If the table matches, and it is not currently matched, add */
        if (match == NULL) {
            // add_table(world, query, table);

        /* If table still matches and has cascade column, reevaluate the
         * sources of references. This may have changed in case 
         * components were added/removed to container entities */ 
        } else if (query->cascade_by) {
            resolve_cascade_subject(world, query, match, table, table->type);

        /* If query has optional columns, it is possible that a column that
         * previously had data no longer has data, or vice versa. Do a
         * rematch to make sure data is consistent. */
        } else if (query->flags & EcsQueryHasOptional) {
            /* Check if optional terms that weren't matched before are matched
             * now & vice versa */
            ecs_query_table_match_t *qt = match->first;

            bool rematch = false;
            int32_t i, count = query->filter.term_count_actual;
            for (i = 0; i < count; i ++) {
                ecs_term_t *term = &query->filter.terms[i];

                if (term->oper == EcsOptional) {
                    int32_t t = term->index;
                    int32_t column = 0;
                    flecs_term_match_table(world, term, table,
                        table->type, 0, &column, 0, 0, true);
                    if (column && (qt->columns[t] == 0)) {
                        rematch = true;
                    } else if (!column && (qt->columns[t] != 0)) {
                        rematch = true;
                    }
                }
            }

            if (rematch) {
                unmatch_table(query, table);
                if (!(query->flags & EcsQueryIsSubquery)) {
                    flecs_table_notify(world, table, &(ecs_table_event_t){
                        .kind = EcsTableQueryUnmatch,
                        .query = query
                    }); 
                }
                // add_table(world, query, table);
            }
        }
    } else {
        /* Table no longer matches, remove */
        if (match != NULL) {
            unmatch_table(query, table);
            if (!(query->flags & EcsQueryIsSubquery)) {
                flecs_table_notify(world, table, &(ecs_table_event_t){
                    .kind = EcsTableQueryUnmatch,
                    .query = query
                });
            }
            notify_subqueries(world, query, &(ecs_query_event_t){
                .kind = EcsQueryTableUnmatch,
                .table = table
            });
        }
    }
}

static
bool satisfy_constraints(
    ecs_world_t *world,
    const ecs_filter_t *filter)
{
    ecs_term_t *terms = filter->terms;
    int32_t i, count = filter->term_count;

    for (i = 0; i < count; i ++) {
        ecs_term_t *term = &terms[i];
        ecs_term_id_t *subj = &term->subj;
        ecs_oper_kind_t oper = term->oper;

        if (oper == EcsOptional) {
            continue;
        }

        if (subj->entity != EcsThis && subj->set.mask & EcsSelf) {
            ecs_type_t type = ecs_get_type(world, subj->entity);

            if (ecs_type_has_id(world, type, term->id, false)) {
                if (oper == EcsNot) {
                    return false;
                }
            } else {
                if (oper != EcsNot) {
                    return false;
                }
            }
        }
    }

    return true;
}

/* Rematch system with tables after a change happened to a watched entity */
static
void rematch_tables(
    ecs_world_t *world,
    ecs_query_t *query,
    ecs_query_t *parent_query)
{    
    if (parent_query) {
        ecs_query_table_t *tables = ecs_vector_first(
            parent_query->cache.tables, ecs_query_table_t);
        int32_t i, count = ecs_vector_count(parent_query->cache.tables);
        for (i = 0; i < count; i ++) {
            rematch_table(world, query, tables[i].hdr.table);
        }

        tables = ecs_vector_first(
            parent_query->cache.empty_tables, ecs_query_table_t);
        count = ecs_vector_count(parent_query->cache.empty_tables);
        for (i = 0; i < count; i ++) {
            rematch_table(world, query, tables[i].hdr.table);
        }        
    } else {
        ecs_sparse_t *tables = world->store.tables;
        int32_t i, count = flecs_sparse_count(tables);

        for (i = 0; i < count; i ++) {
            /* Is the system currently matched with the table? */
            ecs_table_t *table = flecs_sparse_get_dense(tables, ecs_table_t, i);
            rematch_table(world, query, table);
        }
    }

    /* Enable/disable system if constraints are (not) met. If the system is
     * already dis/enabled this operation has no side effects. */
    query->constraints_satisfied = satisfy_constraints(world, &query->filter);      
}

static
void remove_subquery(
    ecs_query_t *parent, 
    ecs_query_t *sub)
{
    ecs_assert(parent != NULL, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(sub != NULL, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(parent->subqueries != NULL, ECS_INTERNAL_ERROR, NULL);

    int32_t i, count = ecs_vector_count(parent->subqueries);
    ecs_query_t **sq = ecs_vector_first(parent->subqueries, ecs_query_t*);

    for (i = 0; i < count; i ++) {
        if (sq[i] == sub) {
            break;
        }
    }

    ecs_vector_remove(parent->subqueries, ecs_query_t*, i);
}

/* -- Private API -- */

void flecs_query_notify(
    ecs_world_t *world,
    ecs_query_t *query,
    ecs_query_event_t *event)
{
    bool notify = true;

    switch(event->kind) {
    case EcsQueryTableMatch:
        /* Creation of new table */
        if (match_table(world, query, event->table)) {
            if (query->subqueries) {
                notify_subqueries(world, query, event);
            }
        }
        notify = false;
        break;
    case EcsQueryTableUnmatch:
        /* Deletion of table */
        unmatch_table(query, event->table);
        break;
    case EcsQueryTableRematch:
        /* Rematch tables of query */
        rematch_tables(world, query, event->parent_query);
        break;        
    case EcsQueryTableEmpty:
        /* Table is empty, deactivate */
        update_table(query, event->table, true);
        break;
    case EcsQueryTableNonEmpty:
        /* Table is non-empty, activate */
        update_table(query, event->table, false);
        break;
    case EcsQueryOrphan:
        ecs_assert(query->flags & EcsQueryIsSubquery, ECS_INTERNAL_ERROR, NULL);
        query->flags |= EcsQueryIsOrphaned;
        query->parent = NULL;
        break;
    }

    if (notify) {
        notify_subqueries(world, query, event);
    }
}

static
void query_order_by(
    ecs_world_t *world,
    ecs_query_t *query,
    ecs_entity_t order_by_component,
    ecs_order_by_action_t order_by)
{
    ecs_check(query != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(!(query->flags & EcsQueryIsOrphaned), ECS_INVALID_PARAMETER, NULL);    
    ecs_check(query->flags & EcsQueryNeedsTables, ECS_INVALID_PARAMETER, NULL);

    query->order_by_component = order_by_component;
    query->order_by = order_by;

    ecs_vector_free(query->table_slices);
    query->table_slices = NULL;

    sort_tables(world, query);  

    if (!query->table_slices) {
        build_sorted_tables(query);
    }
error:
    return;
}

static
void query_group_by(
    ecs_query_t *query,
    ecs_entity_t sort_component,
    ecs_group_by_action_t group_by)
{   
    /* Cannot change grouping once a query has been created */
    ecs_check(query->group_by_id == 0, ECS_INVALID_OPERATION, NULL);
    ecs_check(query->group_by == 0, ECS_INVALID_OPERATION, NULL);

    query->group_by_id = sort_component;
    query->group_by = group_by;
    query->groups = ecs_map_new(ecs_query_table_list_t, 16);
error:
    return;
}

/* Implementation for iterable mixin */
static
void query_iter_init(
    const ecs_world_t *world,
    const ecs_poly_t *poly,
    ecs_iter_t *iter,
    ecs_term_t *filter)
{
    ecs_poly_assert(poly, ecs_query_t);

    if (filter) {
        iter[1] = ecs_query_iter(world, (ecs_query_t*)poly);
        iter[0] = ecs_term_chain_iter(&iter[1], filter);
    } else {
        iter[0] = ecs_query_iter(world, (ecs_query_t*)poly);
    }
}

/* -- Public API -- */

ecs_query_t* ecs_query_init(
    ecs_world_t *world,
    const ecs_query_desc_t *desc)
{
    ecs_query_t *result = NULL;
    ecs_check(world != NULL, ECS_INTERNAL_ERROR, NULL);
    ecs_check(desc != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(desc->_canary == 0, ECS_INVALID_PARAMETER, NULL);
    ecs_check(!world->is_fini, ECS_INVALID_OPERATION, NULL);

    result = flecs_sparse_add(world->queries, ecs_query_t);
    ecs_poly_init(result, ecs_query_t);

    result->id = flecs_sparse_last_id(world->queries);

    ecs_filter_desc_t filter_desc = desc->filter;
    filter_desc.filter = true;
    filter_desc.match_empty_tables = true;

    if (ecs_filter_init(world, &result->filter, &filter_desc)) {
        flecs_sparse_remove(world->queries, result->id);
        return NULL;
    }

    result->world = world;
    result->iterable.init = query_iter_init;
    result->system = desc->system;
    result->prev_match_count = -1;

    ecs_table_cache_init(
        &result->cache, ecs_query_table_t, result, remove_table);

    process_signature(world, result);

    /* Group before matching so we won't have to move tables around later */
    int32_t cascade_by = result->cascade_by;
    if (cascade_by) {
        query_group_by(result, result->filter.terms[cascade_by - 1].id,
            group_by_cascade);
        result->group_by_ctx = &result->filter.terms[cascade_by - 1];
    }

    if (desc->group_by) {
        /* Can't have a cascade term and group by at the same time, as cascade
         * uses the group_by mechanism */
        ecs_check(!result->cascade_by, ECS_INVALID_PARAMETER, NULL);
        query_group_by(result, desc->group_by_id, desc->group_by);
        result->group_by_ctx = desc->group_by_ctx;
        result->group_by_ctx_free = desc->group_by_ctx_free;
    }

    if (desc->parent != NULL) {
        result->flags |= EcsQueryIsSubquery;
    }

    /* If a system is specified, ensure that if there are any subjects in the
     * filter that refer to the system, the component is added */
    if (desc->system)  {
        int32_t t, term_count = result->filter.term_count;
        ecs_term_t *terms = result->filter.terms;

        for (t = 0; t < term_count; t ++) {
            ecs_term_t *term = &terms[t];
            if (term->subj.entity == desc->system) {
                ecs_add_id(world, desc->system, term->id);
            }
        }        
    }

#ifndef NDEBUG
    char *filter_expr = ecs_filter_str(world, &result->filter);
    ecs_dbg_1("#[green]query#[normal] [%s] created", filter_expr);
    ecs_os_free(filter_expr);
#endif

    ecs_log_push();

    if (!desc->parent) {
        match_tables(world, result);
    } else {
        add_subquery(world, desc->parent, result);
        result->parent = desc->parent;
    }

    if (desc->order_by) {
        query_order_by(
            world, result, desc->order_by_component, desc->order_by);
    }

    result->constraints_satisfied = satisfy_constraints(world, &result->filter);

    ecs_log_pop();

    return result;
error:
    if (result) {
        flecs_sparse_remove(world->queries, result->id);
    }
    return NULL;
}

void ecs_query_fini(
    ecs_query_t *query)
{
    ecs_poly_assert(query, ecs_query_t);
    ecs_world_t *world = query->world;
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);

    if (query->group_by_ctx_free) {
        if (query->group_by_ctx) {
            query->group_by_ctx_free(query->group_by_ctx);
        }
    }

    if ((query->flags & EcsQueryIsSubquery) &&
        !(query->flags & EcsQueryIsOrphaned))
    {
        remove_subquery(query->parent, query);
    }

    notify_subqueries(world, query, &(ecs_query_event_t){
        .kind = EcsQueryOrphan
    });

    ecs_vector_each(query->cache.empty_tables, ecs_query_table_t, table, {
        if (!(query->flags & EcsQueryIsSubquery)) {
            flecs_table_notify(world, table->hdr.table, &(ecs_table_event_t){
                .kind = EcsTableQueryUnmatch,
                .query = query
            });
        }    
    });

    ecs_vector_each(query->cache.tables, ecs_query_table_t, table, {
        if (!(query->flags & EcsQueryIsSubquery)) {
            flecs_table_notify(world, table->hdr.table, &(ecs_table_event_t){
                .kind = EcsTableQueryUnmatch,
                .query = query
            });
        }
    });

    ecs_table_cache_fini(&query->cache);
    ecs_map_free(query->groups);

    ecs_vector_free(query->subqueries);
    ecs_vector_free(query->table_slices);
    ecs_filter_fini(&query->filter);

    ecs_poly_fini(query, ecs_query_t);
    
    /* Remove query from storage */
    flecs_sparse_remove(world->queries, query->id);
error:
    return;
}

const ecs_filter_t* ecs_query_get_filter(
    ecs_query_t *query)
{
    ecs_poly_assert(query, ecs_query_t);
    return &query->filter;
}

/* Create query iterator */
ecs_iter_t ecs_query_iter(
    const ecs_world_t *stage,
    ecs_query_t *query)
{
    ecs_poly_assert(query, ecs_query_t);
    ecs_check(!(query->flags & EcsQueryIsOrphaned),
        ECS_INVALID_PARAMETER, NULL);

    ecs_world_t *world = (ecs_world_t*)ecs_get_world(stage);

    sort_tables(world, query);

    if (!world->is_readonly && query->flags & EcsQueryHasRefs) {
        flecs_eval_component_monitors(world);
    }

    query->prev_match_count = query->match_count;

    int32_t table_count;
    if (query->table_slices) {
        table_count = ecs_vector_count(query->table_slices);
    } else {
        table_count = ecs_vector_count(query->cache.tables);
    }

    ecs_query_iter_t it = {
        .query = query,
        .node = query->list.first
    };

    if (query->order_by && query->list.count) {
        it.node = ecs_vector_first(query->table_slices, ecs_query_table_node_t);
    }

    return (ecs_iter_t){
        .real_world = world,
        .world = (ecs_world_t*)stage,
        .terms = query->filter.terms,
        .term_count = query->filter.term_count_actual,
        .table_count = table_count,
        .is_filter = query->filter.filter,
        .is_instanced = query->filter.instanced,
        .priv.iter.query = it,
        .next = ecs_query_next,
    };
error:
    return (ecs_iter_t){ 0 };
}

static
int find_smallest_column(
    ecs_table_t *table,
    ecs_query_table_match_t *table_data,
    ecs_vector_t *sparse_columns)
{
    flecs_sparse_column_t *sparse_column_array = 
        ecs_vector_first(sparse_columns, flecs_sparse_column_t);
    int32_t i, count = ecs_vector_count(sparse_columns);
    int32_t min = INT_MAX, index = 0;

    for (i = 0; i < count; i ++) {
        /* The array with sparse queries for the matched table */
        flecs_sparse_column_t *sparse_column = &sparse_column_array[i];

        /* Pointer to the switch column struct of the table */
        ecs_sw_column_t *sc = sparse_column->sw_column;

        /* If the sparse column pointer hadn't been retrieved yet, do it now */
        if (!sc) {
            /* Get the table column index from the signature column index */
            int32_t table_column_index = table_data->columns[
                sparse_column->signature_column_index];

            /* Translate the table column index to switch column index */
            table_column_index -= table->sw_column_offset;
            ecs_assert(table_column_index >= 1, ECS_INTERNAL_ERROR, NULL);

            /* Get the sparse column */
            ecs_data_t *data = &table->storage;
            sc = sparse_column->sw_column = 
                &data->sw_columns[table_column_index - 1];
        }

        /* Find the smallest column */
        ecs_switch_t *sw = sc->data;
        int32_t case_count = flecs_switch_case_count(sw, sparse_column->sw_case);
        if (case_count < min) {
            min = case_count;
            index = i + 1;
        }
    }

    return index;
}

typedef struct {
    int32_t first;
    int32_t count;
} query_iter_cursor_t;

static
int sparse_column_next(
    ecs_table_t *table,
    ecs_query_table_match_t *matched_table,
    ecs_vector_t *sparse_columns,
    ecs_query_iter_t *iter,
    query_iter_cursor_t *cur,
    bool filter)
{
    bool first_iteration = false;
    int32_t sparse_smallest;

    if (!(sparse_smallest = iter->sparse_smallest)) {
        sparse_smallest = iter->sparse_smallest = find_smallest_column(
            table, matched_table, sparse_columns);
        first_iteration = true;
    }

    sparse_smallest -= 1;

    flecs_sparse_column_t *columns = ecs_vector_first(
        sparse_columns, flecs_sparse_column_t);
    flecs_sparse_column_t *column = &columns[sparse_smallest];
    ecs_switch_t *sw, *sw_smallest = column->sw_column->data;
    ecs_entity_t case_smallest = column->sw_case;

    /* Find next entity to iterate in sparse column */
    int32_t first, sparse_first = iter->sparse_first;

    if (!filter) {
        if (first_iteration) {
            first = flecs_switch_first(sw_smallest, case_smallest);
        } else {
            first = flecs_switch_next(sw_smallest, sparse_first);
        }
    } else {
        int32_t cur_first = cur->first, cur_count = cur->count;
        first = cur_first;
        while (flecs_switch_get(sw_smallest, first) != case_smallest) {
            first ++;
            if (first >= (cur_first + cur_count)) {
                first = -1;
                break;
            }
        }
    }

    if (first == -1) {
        goto done;
    }

    /* Check if entity matches with other sparse columns, if any */
    int32_t i, count = ecs_vector_count(sparse_columns);
    do {
        for (i = 0; i < count; i ++) {
            if (i == sparse_smallest) {
                /* Already validated this one */
                continue;
            }

            column = &columns[i];
            sw = column->sw_column->data;

            if (flecs_switch_get(sw, first) != column->sw_case) {
                first = flecs_switch_next(sw_smallest, first);
                if (first == -1) {
                    goto done;
                }
            }
        }
    } while (i != count);

    cur->first = iter->sparse_first = first;
    cur->count = 1;

    return 0;
done:
    /* Iterated all elements in the sparse list, we should move to the
     * next matched table. */
    iter->sparse_smallest = 0;
    iter->sparse_first = 0;

    return -1;
}

#define BS_MAX ((uint64_t)0xFFFFFFFFFFFFFFFF)

static
int bitset_column_next(
    ecs_table_t *table,
    ecs_vector_t *bitset_columns,
    ecs_query_iter_t *iter,
    query_iter_cursor_t *cur)
{
    /* Precomputed single-bit test */
    static const uint64_t bitmask[64] = {
    (uint64_t)1 << 0, (uint64_t)1 << 1, (uint64_t)1 << 2, (uint64_t)1 << 3,
    (uint64_t)1 << 4, (uint64_t)1 << 5, (uint64_t)1 << 6, (uint64_t)1 << 7,
    (uint64_t)1 << 8, (uint64_t)1 << 9, (uint64_t)1 << 10, (uint64_t)1 << 11,
    (uint64_t)1 << 12, (uint64_t)1 << 13, (uint64_t)1 << 14, (uint64_t)1 << 15,
    (uint64_t)1 << 16, (uint64_t)1 << 17, (uint64_t)1 << 18, (uint64_t)1 << 19,
    (uint64_t)1 << 20, (uint64_t)1 << 21, (uint64_t)1 << 22, (uint64_t)1 << 23,
    (uint64_t)1 << 24, (uint64_t)1 << 25, (uint64_t)1 << 26, (uint64_t)1 << 27,  
    (uint64_t)1 << 28, (uint64_t)1 << 29, (uint64_t)1 << 30, (uint64_t)1 << 31,
    (uint64_t)1 << 32, (uint64_t)1 << 33, (uint64_t)1 << 34, (uint64_t)1 << 35,  
    (uint64_t)1 << 36, (uint64_t)1 << 37, (uint64_t)1 << 38, (uint64_t)1 << 39,
    (uint64_t)1 << 40, (uint64_t)1 << 41, (uint64_t)1 << 42, (uint64_t)1 << 43,
    (uint64_t)1 << 44, (uint64_t)1 << 45, (uint64_t)1 << 46, (uint64_t)1 << 47,  
    (uint64_t)1 << 48, (uint64_t)1 << 49, (uint64_t)1 << 50, (uint64_t)1 << 51,
    (uint64_t)1 << 52, (uint64_t)1 << 53, (uint64_t)1 << 54, (uint64_t)1 << 55,  
    (uint64_t)1 << 56, (uint64_t)1 << 57, (uint64_t)1 << 58, (uint64_t)1 << 59,
    (uint64_t)1 << 60, (uint64_t)1 << 61, (uint64_t)1 << 62, (uint64_t)1 << 63
    };

    /* Precomputed test to verify if remainder of block is set (or not) */
    static const uint64_t bitmask_remain[64] = {
    BS_MAX, BS_MAX - (BS_MAX >> 63), BS_MAX - (BS_MAX >> 62),
    BS_MAX - (BS_MAX >> 61), BS_MAX - (BS_MAX >> 60), BS_MAX - (BS_MAX >> 59),
    BS_MAX - (BS_MAX >> 58), BS_MAX - (BS_MAX >> 57), BS_MAX - (BS_MAX >> 56), 
    BS_MAX - (BS_MAX >> 55), BS_MAX - (BS_MAX >> 54), BS_MAX - (BS_MAX >> 53), 
    BS_MAX - (BS_MAX >> 52), BS_MAX - (BS_MAX >> 51), BS_MAX - (BS_MAX >> 50), 
    BS_MAX - (BS_MAX >> 49), BS_MAX - (BS_MAX >> 48), BS_MAX - (BS_MAX >> 47), 
    BS_MAX - (BS_MAX >> 46), BS_MAX - (BS_MAX >> 45), BS_MAX - (BS_MAX >> 44), 
    BS_MAX - (BS_MAX >> 43), BS_MAX - (BS_MAX >> 42), BS_MAX - (BS_MAX >> 41), 
    BS_MAX - (BS_MAX >> 40), BS_MAX - (BS_MAX >> 39), BS_MAX - (BS_MAX >> 38), 
    BS_MAX - (BS_MAX >> 37), BS_MAX - (BS_MAX >> 36), BS_MAX - (BS_MAX >> 35), 
    BS_MAX - (BS_MAX >> 34), BS_MAX - (BS_MAX >> 33), BS_MAX - (BS_MAX >> 32), 
    BS_MAX - (BS_MAX >> 31), BS_MAX - (BS_MAX >> 30), BS_MAX - (BS_MAX >> 29), 
    BS_MAX - (BS_MAX >> 28), BS_MAX - (BS_MAX >> 27), BS_MAX - (BS_MAX >> 26), 
    BS_MAX - (BS_MAX >> 25), BS_MAX - (BS_MAX >> 24), BS_MAX - (BS_MAX >> 23), 
    BS_MAX - (BS_MAX >> 22), BS_MAX - (BS_MAX >> 21), BS_MAX - (BS_MAX >> 20), 
    BS_MAX - (BS_MAX >> 19), BS_MAX - (BS_MAX >> 18), BS_MAX - (BS_MAX >> 17), 
    BS_MAX - (BS_MAX >> 16), BS_MAX - (BS_MAX >> 15), BS_MAX - (BS_MAX >> 14), 
    BS_MAX - (BS_MAX >> 13), BS_MAX - (BS_MAX >> 12), BS_MAX - (BS_MAX >> 11), 
    BS_MAX - (BS_MAX >> 10), BS_MAX - (BS_MAX >> 9), BS_MAX - (BS_MAX >> 8), 
    BS_MAX - (BS_MAX >> 7), BS_MAX - (BS_MAX >> 6), BS_MAX - (BS_MAX >> 5), 
    BS_MAX - (BS_MAX >> 4), BS_MAX - (BS_MAX >> 3), BS_MAX - (BS_MAX >> 2),
    BS_MAX - (BS_MAX >> 1)
    };

    int32_t i, count = ecs_vector_count(bitset_columns);
    flecs_bitset_column_t *columns = ecs_vector_first(
        bitset_columns, flecs_bitset_column_t);
    int32_t bs_offset = table->bs_column_offset;

    int32_t first = iter->bitset_first;
    int32_t last = 0;

    for (i = 0; i < count; i ++) {
        flecs_bitset_column_t *column = &columns[i];
        ecs_bs_column_t *bs_column = columns[i].bs_column;

        if (!bs_column) {
            int32_t index = column->column_index;
            ecs_assert((index - bs_offset >= 0), ECS_INTERNAL_ERROR, NULL);
            bs_column = &table->storage.bs_columns[index - bs_offset];
            columns[i].bs_column = bs_column;
        }
        
        ecs_bitset_t *bs = &bs_column->data;
        int32_t bs_elem_count = bs->count;
        int32_t bs_block = first >> 6;
        int32_t bs_block_count = ((bs_elem_count - 1) >> 6) + 1;

        if (bs_block >= bs_block_count) {
            goto done;
        }

        uint64_t *data = bs->data;
        int32_t bs_start = first & 0x3F;

        /* Step 1: find the first non-empty block */
        uint64_t v = data[bs_block];
        uint64_t remain = bitmask_remain[bs_start];
        while (!(v & remain)) {
            /* If no elements are remaining, move to next block */
            if ((++bs_block) >= bs_block_count) {
                /* No non-empty blocks left */
                goto done;
            }

            bs_start = 0;
            remain = BS_MAX; /* Test the full block */
            v = data[bs_block];
        }

        /* Step 2: find the first non-empty element in the block */
        while (!(v & bitmask[bs_start])) {
            bs_start ++;

            /* Block was not empty, so bs_start must be smaller than 64 */
            ecs_assert(bs_start < 64, ECS_INTERNAL_ERROR, NULL);
        }
        
        /* Step 3: Find number of contiguous enabled elements after start */
        int32_t bs_end = bs_start, bs_block_end = bs_block;
        
        remain = bitmask_remain[bs_end];
        while ((v & remain) == remain) {
            bs_end = 0;
            bs_block_end ++;

            if (bs_block_end == bs_block_count) {
                break;
            }

            v = data[bs_block_end];
            remain = BS_MAX; /* Test the full block */
        }

        /* Step 4: find remainder of enabled elements in current block */
        if (bs_block_end != bs_block_count) {
            while ((v & bitmask[bs_end])) {
                bs_end ++;
            }
        }

        /* Block was not 100% occupied, so bs_start must be smaller than 64 */
        ecs_assert(bs_end < 64, ECS_INTERNAL_ERROR, NULL);

        /* Step 5: translate to element start/end and make sure that each column
         * range is a subset of the previous one. */
        first = bs_block * 64 + bs_start;
        int32_t cur_last = bs_block_end * 64 + bs_end;
        
        /* No enabled elements found in table */
        if (first == cur_last) {
            goto done;
        }

        /* If multiple bitsets are evaluated, make sure each subsequent range
         * is equal or a subset of the previous range */
        if (i) {
            /* If the first element of a subsequent bitset is larger than the
             * previous last value, start over. */
            if (first >= last) {
                i = -1;
                continue;
            }

            /* Make sure the last element of the range doesn't exceed the last
             * element of the previous range. */
            if (cur_last > last) {
                cur_last = last;
            }
        }
        
        last = cur_last;
        int32_t elem_count = last - first;

        /* Make sure last element doesn't exceed total number of elements in 
         * the table */
        if (elem_count > (bs_elem_count - first)) {
            elem_count = (bs_elem_count - first);
            if (!elem_count) {
                iter->bitset_first = 0;
                goto done;
            }
        }
        
        cur->first = first;
        cur->count = elem_count;
        iter->bitset_first = first;
    }
    
    /* Keep track of last processed element for iteration */ 
    iter->bitset_first = last;

    return 0;
done:
    iter->sparse_smallest = 0;
    iter->sparse_first = 0;
    return -1;
}

static
void mark_columns_dirty(
    ecs_query_t *query,
    ecs_query_table_match_t *table_data)
{
    ecs_table_t *table = table_data->table;

    if (table && table->dirty_state) {
        ecs_term_t *terms = query->filter.terms;
        int32_t i, count = query->filter.term_count_actual;
        for (i = 0; i < count; i ++) {
            ecs_term_t *term = &terms[i];
            int32_t ti = term->index;

            if (term->inout == EcsIn || term->inout == EcsInOutFilter) {
                /* Don't mark readonly terms dirty */
                continue;
            }

            if (table_data->subjects[ti] != 0) {
                /* Don't mark table dirty if term is not from the table */
                continue;
            }

            int32_t index = table_data->columns[ti];
            if (index <= 0) {
                /* If term is not set, there's nothing to mark dirty */
                continue;
            }

            /* Potential candidate for marking table dirty, if a component */
            int32_t storage_index = ecs_table_type_to_storage_index(
                table, index - 1);
            if (storage_index >= 0) {
                table->dirty_state[storage_index + 1] ++;
            }
        }
    }
}

bool ecs_query_next(
    ecs_iter_t *it)
{
    ecs_check(it != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(it->next == ecs_query_next, ECS_INVALID_PARAMETER, NULL);

    if (flecs_iter_next_row(it)) {
        return true;
    }

    return flecs_iter_next_instanced(it, ecs_query_next_instanced(it));
error:
    return false;
}

bool ecs_query_next_instanced(
    ecs_iter_t *it)
{
    ecs_check(it != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(it->next == ecs_query_next, ECS_INVALID_PARAMETER, NULL);

    ecs_query_iter_t *iter = &it->priv.iter.query;
    ecs_query_t *query = iter->query;
    ecs_world_t *world = query->world;
    ecs_flags32_t flags = query->flags;
    (void)world;

    it->is_valid = true;

    ecs_poly_assert(world, ecs_world_t);

    if (!query->constraints_satisfied) {
        goto done;
    }

    query_iter_cursor_t cur;
    ecs_query_table_node_t *node, *next, *prev;
    if ((prev = iter->prev)) {
        /* Match has been iterated, update monitor for change tracking */
        if (flags & EcsQueryHasMonitor) {
            sync_match_monitor(query, prev->match);
        }
        if (flags & EcsQueryHasOutColumns) {
            mark_columns_dirty(query, prev->match);
        }
    }

    for (node = iter->node; node != NULL; node = next) {     
        ecs_query_table_match_t *match = node->match;
        ecs_table_t *table = match->table;

        next = node->next;

        if (table) {
            cur.first = node->offset;
            cur.count = node->count;
            if (!cur.count) {
                cur.count = ecs_table_count(table);

                /* List should never contain empty tables */
                ecs_assert(cur.count != 0, ECS_INTERNAL_ERROR, NULL);
            }

            ecs_vector_t *bitset_columns = match->bitset_columns;
            ecs_vector_t *sparse_columns = match->sparse_columns;

            if (bitset_columns || sparse_columns) {
                bool found = false;

                do {
                    found = false;

                    if (bitset_columns) {
                        if (bitset_column_next(table, bitset_columns, iter, 
                            &cur) == -1) 
                        {
                            /* No more enabled components for table */
                            break;
                        } else {
                            found = true;
                            next = node;
                        }
                    }

                    if (sparse_columns) {
                        if (sparse_column_next(table, match,
                            sparse_columns, iter, &cur, found) == -1)
                        {
                            /* No more elements in sparse column */
                            if (found) {
                                /* Try again */
                                next = node->next;
                                found = false;
                            } else {
                                /* Nothing found */
                                break;
                            }
                        } else {
                            found = true;
                            next = node;
                            iter->bitset_first = cur.first + cur.count;
                        }
                    }
                } while (!found);

                if (!found) {
                    continue;
                }
            }
        } else {
            cur.count = 0;
            cur.first = 0;
        }

        it->ids = match->ids;
        it->columns = match->columns;
        it->subjects = match->subjects;
        it->sizes = match->sizes;
        it->references = match->references;
        it->instance_count = 0;

        flecs_iter_init(it);
        flecs_iter_populate_data(world, it, match->table, cur.first, cur.count, 
            it->ptrs, NULL);

        iter->node = next;
        iter->prev = node;
        goto yield;
    }

done:
error:
    ecs_iter_fini(it);
    return false;
    
yield:
    return true;  
}

bool ecs_query_changed(
    ecs_query_t *query,
    ecs_iter_t *it)
{
    if (it) {
        ecs_assert(it->next == ecs_query_next, ECS_INVALID_PARAMETER, NULL);
        ecs_assert(it->is_valid, ECS_INVALID_PARAMETER, NULL);
        ecs_assert(it->count >= it->instance_count, ECS_INVALID_PARAMETER, NULL);

        ecs_query_table_match_t *qt = 
            (ecs_query_table_match_t*)it->priv.iter.query.prev;
        ecs_assert(qt != NULL, ECS_INVALID_PARAMETER, NULL);

        if (!query) {
            query = it->priv.iter.query.query;
        }

        ecs_assert(query != NULL, ECS_INVALID_PARAMETER, NULL);
        ecs_poly_assert(query, ecs_query_t);

        return check_match_monitor(query, qt);
    }

    ecs_poly_assert(query, ecs_query_t);
    ecs_check(!(query->flags & EcsQueryIsOrphaned), 
        ECS_INVALID_PARAMETER, NULL);

    if (!(query->flags & EcsQueryHasMonitor)) {
        query->flags |= EcsQueryHasMonitor;
        init_query_monitors(query);
        return true; /* Monitors didn't exist yet */
    }

    if (query->match_count != query->prev_match_count) {
        return true;
    }

    return check_query_monitor(query);
error:
    return false;
}

void ecs_query_skip(
    ecs_iter_t *it)
{
    ecs_assert(it->next == ecs_query_next, ECS_INVALID_PARAMETER, NULL);
    ecs_assert(it->is_valid, ECS_INVALID_PARAMETER, NULL);
    it->priv.iter.query.prev = NULL;
}

bool ecs_query_orphaned(
    ecs_query_t *query)
{
    ecs_poly_assert(query, ecs_query_t);
    return query->flags & EcsQueryIsOrphaned;
}
