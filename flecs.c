#ifndef FLECS_IMPL
#include "flecs.h"
#endif
#ifndef FLECS_PRIVATE_H
#define FLECS_PRIVATE_H

#ifndef FLECS_TYPES_PRIVATE_H
#define FLECS_TYPES_PRIVATE_H

#ifndef __MACH__
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#endif

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <math.h>

#ifndef FLECS_ENTITY_INDEX_H
#define FLECS_ENTITY_INDEX_H


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

#define ECS_WORLD_MAGIC (0x65637377)
#define ECS_THREAD_MAGIC (0x65637374)

typedef enum ecs_defer_kind_t {
    EcsOpNew,
    EcsOpAdd,
    EcsOpRemove,   
    EcsOpSet,
    EcsOpMut,
    EcsOpModified,
    EcsOpDelete,
    EcsOpClear
} ecs_defer_kind_t;

typedef struct ecs_defer_1_t {
    ecs_entity_t entity;
    void *value;
    ecs_size_t size;
    bool clone_value;
} ecs_defer_1_t;

typedef struct ecs_defer_n_t {
    ecs_entity_t *entities;  
    void **bulk_data;
    int32_t count;
} ecs_defer_n_t;

typedef struct ecs_defer_t {
    ecs_defer_kind_t kind;
    ecs_entity_t scope;
    ecs_entity_t component;
    ecs_entities_t components;
    union {
        ecs_defer_1_t _1;
        ecs_defer_n_t _n;
    } is;
} ecs_defer_t;

typedef enum ecs_table_eventkind_t {
    EcsTableComponentInfo
} ecs_table_eventkind_t;

typedef struct ecs_table_event_t {
    ecs_table_eventkind_t kind;
    ecs_entity_t component;
} ecs_table_event_t;

#define EcsTableHasBuiltins         1u
#define EcsTableHasComponentData    16u
#define EcsTableIsDisabled          64u
#define EcsTableHasCtors            128u
#define EcsTableHasDtors            256u
#define EcsTableHasCopy             512u
#define EcsTableHasMove             1024u
#define EcsTableHasLifecycle        (EcsTableHasCtors | EcsTableHasDtors)
#define EcsTableIsComplex           (EcsTableHasLifecycle)

typedef struct ecs_column_t {
    ecs_vector_t *data;
    int16_t size;
} ecs_column_t;

typedef struct ecs_data_t {
    ecs_vector_t *entities;
    ecs_vector_t *record_ptrs;
    ecs_column_t *columns;
    ecs_lifecycle_t *lc;
    int32_t *dirty_state;
    int32_t alloc_count;
} ecs_data_t;

typedef struct ecs_edge_t {
    ecs_table_t *add;
    ecs_table_t *remove;
} ecs_edge_t;

struct ecs_table_t {
    ecs_type_t type;
    ecs_edge_t *lo_edges;
    ecs_map_t *hi_edges;
    ecs_data_t *data;
    ecs_flags32_t flags;
    int32_t column_count;
    uint32_t id;
};

typedef struct ecs_entity_info_t {
    ecs_record_t *record;
    ecs_table_t *table;
    ecs_data_t *data;
    int32_t row;
    bool is_watched;
} ecs_entity_info_t;

typedef struct ecs_stage_t {
    ecs_world_t *world;
    int32_t defer;
    ecs_vector_t *defer_queue;
    ecs_vector_t *defer_merge_queue;
    ecs_entity_t scope;
    ecs_table_t *scope_table;
    int32_t id;
} ecs_stage_t;

typedef struct ecs_partition_t {
    ecs_sparse_t *tables;
    ecs_map_t *table_map;
    ecs_table_t root;
} ecs_partition_t;

typedef struct ecs_store_t {
    ecs_dense_t *sparse;
    int8_t partition;
    bool initialized;
} ecs_store_t;

typedef struct ecs_thread_t {
    int32_t magic;
    ecs_world_t *world;
    ecs_stage_t *stage;
    ecs_os_thread_t thread;
} ecs_thread_t;

struct ecs_world_t {
    int32_t magic;
    ecs_ptree_t stores;
    ecs_stage_t stage;
    ecs_sparse_t *entity_index;
    ecs_partition_t store;

    bool in_progress;

    ecs_map_t *store_policies;
    ecs_lifecycle_t lc_lo[ECS_HI_COMPONENT_ID];
    ecs_map_t *lc_hi;

    ecs_world_info_t stats;
};

#endif


////////////////////////////////////////////////////////////////////////////////
//// Core bootstrap functions
////////////////////////////////////////////////////////////////////////////////

#define ECS_TYPE_DECL(component)\
static const ecs_entity_t __##component = ecs_typeid(component);\
ECS_VECTOR_DECL(FLECS__T##component, ecs_entity_t, 1)

#define ECS_TYPE_IMPL(component)\
ECS_VECTOR_IMPL(FLECS__T##component, ecs_entity_t, &__##component, 1)

/* Bootstrap world */
void ecs_bootstrap(
    ecs_world_t *world);

ecs_type_t ecs_bootstrap_type(
    ecs_world_t *world,
    ecs_entity_t entity);

#define ecs_bootstrap_component(world, name)\
    ecs_new_component(world, ecs_typeid(name), #name, sizeof(name), ECS_ALIGNOF(name))

#define ecs_bootstrap_tag(world, name)\
    ecs_set(world, name, EcsName, {.value = &#name[ecs_os_strlen("Ecs")], .symbol = #name});\
    ecs_add_id(world, name, ecs_pair(EcsScope, ecs_get_scope(world)))


////////////////////////////////////////////////////////////////////////////////
//// Entity API
////////////////////////////////////////////////////////////////////////////////

/* Mark an entity as being watched. This is used to trigger automatic rematching
 * when entities used in system expressions change their components. */
void ecs_set_watch(
    ecs_world_t *world,
    ecs_entity_t entity);

/* Does one of the entity containers has specified component */
ecs_entity_t ecs_find_in_type(
    ecs_world_t *world,
    ecs_type_t table_type,
    ecs_entity_t component,
    ecs_entity_t flags);

/* Obtain entity info */
bool ecs_get_info(
    const ecs_world_t *world,
    ecs_entity_t entity,
    ecs_entity_info_t *info);

void ecs_run_monitors(
    ecs_world_t *world, 
    ecs_table_t *dst_table,
    ecs_vector_t *v_dst_monitors, 
    int32_t dst_row, 
    int32_t count, 
    ecs_vector_t *v_src_monitors);

const char* ecs_role_str(
    ecs_entity_t entity);

size_t ecs_entity_str(
    ecs_world_t *world,
    ecs_entity_t entity,
    char *buffer,
    size_t buffer_len);


////////////////////////////////////////////////////////////////////////////////
//// World API
////////////////////////////////////////////////////////////////////////////////

/* Notify systems that there is a new table, which triggers matching */
void ecs_notify_queries_of_table(
    ecs_world_t *world,
    ecs_table_t *table);

/* Get current thread-specific stage */
#ifdef FLECS_THREADING
ecs_stage_t* ecs_get_stage(
    ecs_world_t **world_ptr);
#define ecs_unwrap_world(world_ptr)\
    ecs_get_stage(world_ptr)
#else
#define ecs_get_stage(world_ptr) (&(*world_ptr)->stage)
#define ecs_unwrap_world(world_ptr)
#endif

/* Get component callbacks */
ecs_lifecycle_t* ecs_get_lifecycle(
    ecs_world_t *world,
    ecs_entity_t component);

/* Get or create component callbacks */
ecs_lifecycle_t* ecs_get_or_create_lifecycle(
    ecs_world_t *world,
    ecs_entity_t component);

bool ecs_defer_op_begin(
    ecs_world_t *world,
    ecs_stage_t *stage,
    ecs_defer_kind_t op_kind,
    ecs_entity_t entity,
    ecs_entities_t *components,
    const void *value,
    ecs_size_t size);

void ecs_defer_flush(
    ecs_world_t *world,
    ecs_stage_t *stage);
    
bool ecs_component_has_actions(
    ecs_world_t *world,
    ecs_entity_t component);    

ecs_store_t* ecs_store_ensure(
    ecs_world_t *world,
    ecs_entity_t id);

ecs_store_t* ecs_store_get(
    const ecs_world_t *world,
    ecs_entity_t id);    

////////////////////////////////////////////////////////////////////////////////
//// Stage API
////////////////////////////////////////////////////////////////////////////////

/* Initialize stage data structures */
void ecs_stage_init(
    ecs_world_t *world,
    ecs_stage_t *stage);

/* Deinitialize stage */
void ecs_stage_deinit(
    ecs_world_t *world,
    ecs_stage_t *stage);

/* Merge stage with main stage */
void ecs_stage_merge(
    ecs_world_t *world,
    ecs_stage_t *stage);

/* Begin defer for stage */
void ecs_stage_defer_begin(
    ecs_world_t *world,
    ecs_stage_t *stage);

void ecs_stage_defer_end(
    ecs_world_t *world,
    ecs_stage_t *stage);    

/* Delete table from stage */
void ecs_delete_table(
    ecs_world_t *world,
    ecs_table_t *table);


////////////////////////////////////////////////////////////////////////////////
//// Defer API
////////////////////////////////////////////////////////////////////////////////

bool ecs_defer_none(
    ecs_world_t *world,
    ecs_stage_t *stage);

bool ecs_defer_modified(
    ecs_world_t *world,
    ecs_stage_t *stage,
    ecs_entity_t entity,
    ecs_entity_t component);

bool ecs_defer_new(
    ecs_world_t *world,
    ecs_stage_t *stage,
    ecs_entity_t entity,
    ecs_entities_t *components);

bool ecs_defer_delete(
    ecs_world_t *world,
    ecs_stage_t *stage,
    ecs_entity_t entity);

bool ecs_defer_clear(
    ecs_world_t *world,
    ecs_stage_t *stage,
    ecs_entity_t entity);

bool ecs_defer_add(
    ecs_world_t *world,
    ecs_stage_t *stage,
    ecs_entity_t entity,
    ecs_entity_t component);

bool ecs_defer_remove(
    ecs_world_t *world,
    ecs_stage_t *stage,
    ecs_entity_t entity,
    ecs_entity_t component);

bool ecs_defer_set(
    ecs_world_t *world,
    ecs_stage_t *stage,
    ecs_defer_kind_t op_kind,
    ecs_entity_t entity,
    ecs_entity_t component,
    ecs_size_t size,
    const void *value,
    void **value_out);


////////////////////////////////////////////////////////////////////////////////
//// Table API
////////////////////////////////////////////////////////////////////////////////

/** Find or create table for a set of components */
ecs_table_t* ecs_table_find_or_create(
    ecs_world_t *world,
    ecs_entities_t *type);

/** Find or create table for a type */
ecs_table_t* ecs_table_from_type(
    ecs_world_t *world,
    ecs_type_t type);    

/* Get table data */
ecs_data_t *ecs_table_get_data(
    ecs_table_t *table);

/* Get or create data */
ecs_data_t *ecs_table_get_or_create_data(
    ecs_table_t *table);

/* Initialize columns for data */
ecs_data_t* ecs_init_data(
    ecs_world_t *world,
    ecs_table_t *table,
    ecs_data_t *result); 

/* Clear all entities from a table. */
void ecs_table_clear(
    ecs_world_t *world,
    ecs_table_t *table);

/* Reset a table to its initial state */
void ecs_table_reset(
    ecs_world_t *world,
    ecs_table_t *table);

/* Clear all entities from the table. Do not invoke OnRemove systems */
void ecs_table_clear_silent(
    ecs_world_t *world,
    ecs_table_t *table);

/* Clear table data. Don't call OnRemove handlers. */
void ecs_table_clear_data(
    ecs_table_t *table,
    ecs_data_t *data);    

/* Return number of entities in table. */
int32_t ecs_table_count(
    ecs_table_t *table);

/* Return number of entities in data */
int32_t ecs_table_data_count(
    ecs_data_t *data);

/* Add a new entry to the table for the specified entity */
int32_t ecs_table_append(
    ecs_world_t *world,
    ecs_table_t *table,
    ecs_data_t *data,
    ecs_entity_t entity,
    ecs_record_t *record,
    bool construct);

/* Delete an entity from the table. */
void ecs_table_delete(
    ecs_world_t *world,
    ecs_table_t *table,
    ecs_data_t *data,
    int32_t index,
    bool destruct);

/* Move a row from one table to another */
void ecs_table_move(
    ecs_world_t *world,
    ecs_entity_t dst_entity,
    ecs_entity_t src_entity,
    ecs_table_t *new_table,
    ecs_data_t *new_data,
    int32_t new_index,
    ecs_table_t *old_table,
    ecs_data_t *old_data,
    int32_t old_index);

/* Grow table with specified number of records. Populate table with entities,
 * starting from specified entity id. */
int32_t ecs_table_appendn(
    ecs_world_t *world,
    ecs_table_t *table,
    ecs_data_t *data,
    int32_t count,
    const ecs_entity_t *ids);

/* Set table to a fixed size. Useful for preallocating memory in advance. */
void ecs_table_set_size(
    ecs_world_t *world,
    ecs_table_t *table,
    ecs_data_t *data,
    int32_t count);

/* Get dirty state for table columns */
int32_t* ecs_table_get_dirty_state(
    ecs_table_t *table);

/* Initialize root table */
void ecs_init_root_table(
    ecs_world_t *world,
    ecs_table_t *root);

/* Free table */
void ecs_table_free(
    ecs_world_t *world,
    ecs_table_t *table); 

ecs_table_t *ecs_table_traverse_add(
    ecs_world_t *world,
    ecs_table_t *table,
    ecs_entity_t to_add);

ecs_table_t *ecs_table_traverse_remove(
    ecs_world_t *world,
    ecs_table_t *table,
    ecs_entity_t to_remove);

void ecs_table_mark_dirty(
    ecs_table_t *table,
    ecs_entity_t component);

int32_t ecs_table_switch_from_case(
    ecs_world_t *world,
    ecs_table_t *table,
    ecs_entity_t add);    

void ecs_table_clear_edges(
    ecs_table_t *table);

void ecs_table_notify(
    ecs_world_t * world,
    ecs_table_t * table,
    ecs_table_event_t * event);

int32_t* ecs_table_get_monitor(
    ecs_table_t *table);

////////////////////////////////////////////////////////////////////////////////
//// Time API
////////////////////////////////////////////////////////////////////////////////

void ecs_os_time_setup(void);

uint64_t ecs_os_time_now(void);

void ecs_os_time_sleep(
    int32_t sec, 
    int32_t nanosec);

/* Increase or reset timer resolution (Windows only) */
FLECS_API
void ecs_increase_timer_resolution(
    bool enable);


////////////////////////////////////////////////////////////////////////////////
//// Utilities
////////////////////////////////////////////////////////////////////////////////

void ecs_hash(
    const void *data,
    ecs_size_t length,
    uint64_t *result);

int32_t ecs_next_pow_of_2(
    int32_t n);

/* Convert 64 bit signed integer to 16 bit */
int8_t ecs_to_i8(
    int64_t v);

/* Convert 64 bit signed integer to 16 bit */
int16_t ecs_to_i16(
    int64_t v);

/* Convert 64 bit unsigned integer to 32 bit */
uint32_t ecs_to_u32(
    uint64_t v);        

/* Convert 64 bit unsigned integer to signed 32 bit */
int32_t ecs_to_i32(
    uint64_t size);

/* Convert signed integer to size_t */
size_t ecs_to_size_t(
    int64_t size);


/* Convert size_t to ecs_size_t */
ecs_size_t ecs_from_size_t(
    size_t size);    

/* Convert 64bit value to ecs_record_t type. ecs_record_t is stored as 64bit int in the
 * entity index */
ecs_record_t ecs_to_row(
    uint64_t value);

/* Get 64bit integer from ecs_record_t */
uint64_t ecs_from_row(
    ecs_record_t record);

/* Get actual row from record row */
int32_t ecs_record_to_row(
    int32_t row, 
    bool *is_watched_out);

/* Convert actual row to record row */
int32_t ecs_row_to_record(
    int32_t row, 
    bool is_watched);

/* Convert type to entity array */
ecs_entities_t ecs_type_to_entities(
    ecs_type_t type); 

/* Convert a symbol name to an entity name by removing the prefix */
const char* ecs_name_from_symbol(
    ecs_world_t *world,
    const char *type_name); 

/* Lookup an entity by name with a specific id */
ecs_entity_t ecs_lookup_w_id(
    ecs_world_t *world,
    ecs_entity_t e,
    const char *name);

/* Set entity name with symbol */
void ecs_set_symbol(
    ecs_world_t *world,
    ecs_entity_t e,
    const char *name);

#define assert_func(cond) _assert_func(cond, #cond, __FILE__, __LINE__, __func__)
void _assert_func(
    bool cond,
    const char *cond_str,
    const char *file,
    int32_t line,
    const char *func);

#endif

static
char *ecs_vasprintf(
    const char *fmt,
    va_list args)
{
    ecs_size_t size = 0;
    char *result  = NULL;
    va_list tmpa;

    va_copy(tmpa, args);

    size = vsnprintf(result, ecs_to_size_t(size), fmt, tmpa);

    va_end(tmpa);

    if ((int32_t)size < 0) { 
        return NULL; 
    }

    result = (char *) ecs_os_malloc(size + 1);

    if (!result) { 
        return NULL; 
    }

    ecs_os_vsprintf(result, fmt, args);

    return result;
}

static int trace_indent = 0;
static int trace_level = 0;

static
void ecs_log_print(
    int level,
    const char *file,
    int32_t line,
    const char *fmt,
    va_list valist)
{
    (void)level;
    (void)line;

    if (level > trace_level) {
        return;
    }

    /* Massage filename so it doesn't take up too much space */
    char filebuff[256];
    ecs_os_strcpy(filebuff, file);
    file = filebuff;
    char *file_ptr = strrchr(file, '/');
    if (file_ptr) {
        file = file_ptr + 1;
    }

    /* Extension is likely the same for all files */
    file_ptr = strrchr(file, '.');
    if (file_ptr) {
        *file_ptr = '\0';
    }

    char indent[32];
    int i;
    for (i = 0; i < trace_indent; i ++) {
        indent[i * 2] = '|';
        indent[i * 2 + 1] = ' ';
    }
    indent[i * 2] = '\0';

    char *msg = ecs_vasprintf(fmt, valist);

    if (level >= 0) {
        ecs_os_log("%sinfo%s: %s%s%s%s",
            ECS_MAGENTA, ECS_NORMAL, ECS_GREY, indent, ECS_NORMAL, msg);
    } else if (level == -2) {
        ecs_os_warn("%swarn%s: %s%s%s%s",
            ECS_YELLOW, ECS_NORMAL, ECS_GREY, indent, ECS_NORMAL, msg);
    } else if (level <= -2) {
        ecs_os_err("%serr %s: %s%s%s%s",
            ECS_RED, ECS_NORMAL, ECS_GREY, indent, ECS_NORMAL, msg);
    }

    ecs_os_free(msg);
}

void _ecs_trace(
    int level,
    const char *file,
    int32_t line,
    const char *fmt,
    ...)
{
    va_list valist;
    va_start(valist, fmt);

    ecs_log_print(level, file, line, fmt, valist);
}

void _ecs_warn(
    const char *file,
    int32_t line,
    const char *fmt,
    ...)
{
    va_list valist;
    va_start(valist, fmt);

    ecs_log_print(-2, file, line, fmt, valist);
}

void _ecs_err(
    const char *file,
    int32_t line,
    const char *fmt,
    ...)
{
    va_list valist;
    va_start(valist, fmt);

    ecs_log_print(-3, file, line, fmt, valist);
}

void ecs_log_push(void) {
    trace_indent ++;
}

void ecs_log_pop(void) {
    trace_indent --;
}

void ecs_tracing_enable(
    int level)
{
    trace_level = level;
}

static
const char* strip_file(
    const char *file)
{
    const char *f = strrchr(file, '/');
    if (f) {
        return f + 1;
    } else {
        return file;
    }
}

void _ecs_abort(
    int32_t error_code,
    const char *file,
    int32_t line)
{
    ecs_err("abort %s:%d: %s", strip_file(file), line, ecs_strerror(error_code));
    ecs_os_abort();
}

void _ecs_assert(
    bool condition,
    int32_t error_code,
    const char *condition_str,
    const char *file,
    int32_t line)
{
    if (!condition) {
        ecs_err("assert(%s) %s:%d: %s", condition_str, strip_file(file), line, 
            ecs_strerror(error_code));
        ecs_os_abort();
    }
}

const char* ecs_strerror(
    int32_t error_code)
{
    switch (error_code) {

    case ECS_INTERNAL_ERROR:
        return "ECS_INTERNAL_ERROR";
    case ECS_INVALID_OPERATION:
        return "ECS_INVALID_OPERATION";
    case ECS_INVALID_PARAMETER:
        return "ECS_INVALID_PARAMETER";
    case ECS_INVALID_ID:
        return "ECS_INVALID_ID";
    case ECS_INVALID_COMPONENT:
        return "ECS_INVALID_COMPONENT";
    case ECS_OUT_OF_MEMORY:
        return "ECS_OUT_OF_MEMORY";
    case ECS_MISSING_OS_API:
        return "ECS_MISSING_OS_API";
    case ECS_INCONSISTENT_COMPONENT_ACTION:
        return "ECS_INCONSISTENT_COMPONENT_ACTION";
    case ECS_INVALID_FROM_WORKER:
        return "ECS_INVALID_FROM_WORKER";
    }

    return "unknown error code";
}

static
ecs_flags32_t get_component_action_flags(
    ecs_lifecycle_t *lc) 
{
    ecs_flags32_t flags = 0;

    if (lc->ctor) {
        flags |= EcsTableHasCtors;
    }
    if (lc->dtor) {
        flags |= EcsTableHasDtors;
    }
    if (lc->copy) {
        flags |= EcsTableHasCopy;
    }
    if (lc->move) {
        flags |= EcsTableHasMove;
    } 

    return flags;  
}

/* Check if table has instance of component, including traits */
static
bool has_component(
    ecs_world_t *world,
    ecs_type_t type,
    ecs_entity_t component)
{
    ecs_entity_t *entities = ecs_vector_first(type, ecs_entity_t);
    int32_t i, count = ecs_vector_count(type);

    for (i = 0; i < count; i ++) {
        if (component == ecs_get_type_id_from_id(world, entities[i])) {
            return true;
        }
    }
    
    return false;
}

static
void notify_component_info(
    ecs_world_t *world,
    ecs_table_t *table,
    ecs_entity_t component)
{
    ecs_type_t table_type = table->type;
    if (!component || has_component(world, table_type, component)){
        int32_t column_count = ecs_vector_count(table_type);
        ecs_assert(!component || column_count != 0, ECS_INTERNAL_ERROR);

        if (!column_count) {
            return;
        }

        ecs_data_t *data = ecs_table_get_data(table);
        if (!data) {
            return;
        }
        
        if (!data->lc) {
            data->lc = ecs_os_calloc(
                ECS_SIZEOF(ecs_lifecycle_t) * column_count);
        }

        /* Reset lifecycle flags before recomputing */
        table->flags &= ~EcsTableHasLifecycle;

        /* Recompute lifecycle flags */
        ecs_entity_t *array = ecs_vector_first(table_type, ecs_entity_t);
        int32_t i;
        for (i = 0; i < column_count; i ++) {
            ecs_entity_t c = ecs_get_type_id_from_id(world, array[i]);
            if (!c) {
                continue;
            }
            
            ecs_lifecycle_t *lc = ecs_get_lifecycle(world, c);
            if (lc) {
                ecs_flags32_t flags = get_component_action_flags(lc);
                table->flags |= flags;
                data->lc[i] = *lc;
            }            
        }        
    }
}


ecs_data_t* ecs_init_data(
    ecs_world_t *world,
    ecs_table_t *table,
    ecs_data_t *result)
{
    ecs_type_t type = table->type; 
    int32_t i, count = table->column_count;

    /* Root tables don't have columns */
    if (!count) {
        result->columns = NULL;
        return result;
    }

    /* When a table is going to store data, initialize the cache with component
     * lifecycle callbacks */
    notify_component_info(world, table, 0);

    ecs_entity_t *entities = ecs_vector_first(type, ecs_entity_t);

    if (count) {
        result->columns = ecs_os_calloc(ECS_SIZEOF(ecs_column_t) * count);    
    } else if (count) {
        /* If a table has switch columns, store vector with the case values
            * as a regular column, so it's easier to access for systems. To
            * enable this, we need to allocate more space. */
        int32_t type_count = ecs_vector_count(type);
        result->columns = ecs_os_calloc(ECS_SIZEOF(ecs_column_t) * type_count);
    }

    if (count) {
        for (i = 0; i < count; i ++) {
            ecs_entity_t e = entities[i];

            /* Is the column a component? */
            const EcsType *component = ecs_get_type_from_id(world, e);
            if (component) {
                /* Is the component associated wit a (non-empty) type? */
                if (component->size) {
                    /* This is a regular component column */
                    result->columns[i].size = ecs_to_i16(component->size);
                } else {
                    /* This is a tag */
                }
            } else {
                /* This is an entity that was added to the type */
            }
        }
    }

    return result;
}

/* -- Private functions -- */

static
ecs_data_t* get_data_intern(
    ecs_table_t *table,
    bool create)
{
    ecs_assert(table != NULL, ECS_INTERNAL_ERROR);

    ecs_data_t *data = table->data;
    if (data) {
        return data;
    }

    if (!data && !create) {
        return NULL;
    }

    return table->data = ecs_os_calloc(ECS_SIZEOF(ecs_data_t));
}

ecs_data_t* ecs_table_get_data(
    ecs_table_t *table)
{
    return get_data_intern(table, false);
}

ecs_data_t* ecs_table_get_or_create_data(
    ecs_table_t *table)
{
    return get_data_intern(table, true);
}

static
void ctor_component(
    ecs_lifecycle_t * lc,
    ecs_column_t * column,
    int32_t row,
    int32_t count)
{
    ecs_xtor_t ctor;
    if (lc && (ctor = lc->ctor)) {
        int16_t size = column->size;
        void *ptr = _ecs_vector_get(column->data, size, row);
        ecs_assert(ptr != NULL, ECS_INTERNAL_ERROR);
        ctor(ptr, ecs_to_size_t(size), count);
    }
}

static
void dtor_component(
    ecs_lifecycle_t * lc,
    ecs_column_t * column,
    int32_t row,
    int32_t count)
{
    ecs_xtor_t dtor;
    if (lc && (dtor = lc->dtor)) {
        int16_t size = column->size;
        void *ptr = _ecs_vector_get(column->data, size, row);
        ecs_assert(ptr != NULL, ECS_INTERNAL_ERROR);
        dtor(ptr, ecs_to_size_t(size), count);
    }
}

static
void dtor_all_components(
    ecs_table_t * table,
    ecs_data_t * data,
    int32_t row,
    int32_t count)
{
    int32_t column_count = table->column_count;
    int32_t i;
    for (i = 0; i < column_count; i ++) {
        ecs_column_t *column = &data->columns[i];
        dtor_component(&data->lc[i], column, row, count);
    }
}

static
void run_remove_actions(
    ecs_table_t * table,
    ecs_data_t * data,
    int32_t row,
    int32_t count)
{
    if (count) {
        dtor_all_components(table, data, row, count);
    }
}

void ecs_table_clear_data(
    ecs_table_t * table,
    ecs_data_t * data)
{
    if (!data) {
        return;
    }
    
    ecs_column_t *columns = data->columns;
    if (columns) {
        int32_t c, column_count = table->column_count;
        for (c = 0; c < column_count; c ++) {
            ecs_vector_free(columns[c].data);
        }
        ecs_os_free(columns);
        data->columns = NULL;
    }

    ecs_vector_free(data->entities);
    ecs_vector_free(data->record_ptrs);
    ecs_os_free(data->dirty_state);

    data->entities = NULL;
    data->record_ptrs = NULL;
}

/* Clear columns. Deactivate table in systems if necessary, but do not invoke
 * OnRemove handlers. This is typically used when restoring a table to a
 * previous state. */
void ecs_table_clear_silent(
    ecs_world_t * world,
    ecs_table_t * table)
{
    (void)world;

    ecs_data_t *data = ecs_table_get_data(table);
    if (!data) {
        return;
    }

    int32_t count = ecs_vector_count(data->entities);
    
    ecs_table_clear_data(table, table->data);

    if (count) {
        // ecs_table_activate(world, table, 0, false);
    }
}

/* Delete all entities in table, invoke OnRemove handlers. This function is used
 * when an application invokes delete_w_filter. Use ecs_table_clear_silent, as 
 * the table may have to be deactivated with systems. */
void ecs_table_clear(
    ecs_world_t * world,
    ecs_table_t * table)
{
    ecs_data_t *data = ecs_table_get_data(table);
    if (data) {
        run_remove_actions(table, data, 0, ecs_table_data_count(data));

        ecs_entity_t *entities = ecs_vector_first(data->entities, ecs_entity_t);
        int32_t i, count = ecs_vector_count(data->entities);
        for(i = 0; i < count; i ++) {
            ecs_eis_delete(world, entities[i]);
        }
    }

    ecs_table_clear_silent(world, table);
}

/* Free table resources. Do not invoke handlers and do not activate/deactivate
 * table with systems. This function is used when the world is freed. */
void ecs_table_free(
    ecs_world_t * world,
    ecs_table_t * table)
{
    (void)world;
    ecs_data_t *data = ecs_table_get_data(table);
    if (data) {
        run_remove_actions(table, data, 0, ecs_table_data_count(data));
        if (data->lc) {
            ecs_os_free(data->lc);
        }            
    }

    ecs_table_clear_data(table, table->data);
    ecs_table_clear_edges(table);
    
    ecs_os_free(table->lo_edges);
    ecs_map_free(table->hi_edges);
    ecs_vector_free((ecs_vector_t*)table->type);

    table->id = 0;

    ecs_os_free(table->data);
}

/* Reset a table to its initial state. */
void ecs_table_reset(
    ecs_world_t * world,
    ecs_table_t * table)
{
    (void)world;

    ecs_os_free(table->lo_edges);
    ecs_map_free(table->hi_edges);
    table->lo_edges = NULL;
    table->hi_edges = NULL;
}

static
void mark_dirty(
    ecs_data_t *data,
    int32_t index)
{
    if (data->dirty_state) {
        data->dirty_state[index] ++;
    }
}

void ecs_table_mark_dirty(
    ecs_table_t *table,
    ecs_entity_t component)
{
    ecs_assert(table != NULL, ECS_INTERNAL_ERROR);
    ecs_assert(table->data != NULL, ECS_INTERNAL_ERROR);
    
    ecs_data_t *data = table->data;
    if (data->dirty_state) {
        int32_t index = ecs_type_index_of(table->type, component);
        ecs_assert(index != -1, ECS_INTERNAL_ERROR);
        mark_dirty(data, index);
    }
}

static
void ensure_data(
    ecs_world_t * world,
    ecs_table_t * table,
    ecs_data_t * data,
    int32_t * column_count_out,
    ecs_column_t ** columns_out)
{
    int32_t column_count = table->column_count;
    ecs_column_t *columns = NULL;

    /* It is possible that the table data was created without content. 
     * Now that data is going to be written to the table, initialize */ 
    if (column_count) {
        columns = data->columns;

        if (!columns) {
            ecs_init_data(world, table, data);
            columns = data->columns;
        }

        *column_count_out = column_count;
        *columns_out = columns;
    }
}

static
void grow_column(
    ecs_column_t * column,
    ecs_lifecycle_t * lc,
    int32_t to_add,
    int32_t new_size,
    bool construct)
{
    ecs_vector_t *vec = column->data;

    int32_t size = column->size;
    int32_t count = ecs_vector_count(vec);
    int32_t old_size = ecs_vector_size(vec);
    int32_t new_count = count + to_add;
    bool can_realloc = new_size != old_size;

    ecs_assert(new_size >= new_count, ECS_INTERNAL_ERROR);

    /* If the array could possibly realloc and the component has a move action 
     * defined, move old elements manually */
    ecs_move_t move;
    if (lc && count && can_realloc && (move = lc->move)) {
        ecs_xtor_t ctor = lc->ctor;
        ecs_assert(ctor != NULL, ECS_INTERNAL_ERROR);

        /* Create new vector */
        ecs_vector_t *new_vec = _ecs_vector_new(size, new_size);
        _ecs_vector_set_count(&new_vec, size, new_count);

        void *old_buffer = _ecs_vector_first(vec, size);

        void *new_buffer = _ecs_vector_first(new_vec, size);

        /* First construct elements (old and new) in new buffer */
        ctor(new_buffer, ecs_to_size_t(size), construct ? new_count : count);
        
        /* Move old elements */
        move(new_buffer, old_buffer, ecs_to_size_t(size), count);

        /* Free old vector */
        ecs_vector_free(vec);
        column->data = new_vec;
    } else {
        /* If array won't realloc or has no move, simply add new elements */
        if (can_realloc) {
            _ecs_vector_set_size(&vec, size, new_size);
        }

        void *elem = _ecs_vector_addn(&vec, size, to_add);

        ecs_xtor_t ctor;
        if (construct && lc && (ctor = lc->ctor)) {
            /* If new elements need to be constructed and component has a
             * constructor, construct */
            ctor(elem, ecs_to_size_t(size), to_add);
        }

        column->data = vec;
    }

    ecs_assert(ecs_vector_size(column->data) == new_size, 
        ECS_INTERNAL_ERROR);
}

static
int32_t grow_data(
    ecs_world_t * world,
    ecs_table_t * table,
    ecs_data_t * data,
    int32_t to_add,
    int32_t size,
    const ecs_entity_t *ids)
{
    ecs_assert(table != NULL, ECS_INTERNAL_ERROR);
    ecs_assert(data != NULL, ECS_INTERNAL_ERROR);

    int32_t cur_count = ecs_table_data_count(data);
    int32_t column_count = table->column_count;
    ecs_column_t *columns;
    ensure_data(world, table, data, &column_count, &columns);

    /* Add record to record ptr array */
    ecs_vector_set_size(&data->record_ptrs, ecs_record_t*, size);
    ecs_record_t **r = ecs_vector_addn(&data->record_ptrs, ecs_record_t*, to_add);
    ecs_assert(r != NULL, ECS_INTERNAL_ERROR);
    if (ecs_vector_size(data->record_ptrs) > size) {
        size = ecs_vector_size(data->record_ptrs);
    }

    /* Add entity to column with entity ids */
    ecs_vector_set_size(&data->entities, ecs_entity_t, size);
    ecs_entity_t *e = ecs_vector_addn(&data->entities, ecs_entity_t, to_add);
    ecs_assert(e != NULL, ECS_INTERNAL_ERROR);
    ecs_assert(ecs_vector_size(data->entities) == size, ECS_INTERNAL_ERROR);

    /* Initialize entity ids and record ptrs */
    int32_t i;
    if (ids) {
        for (i = 0; i < to_add; i ++) {
            e[i] = ids[i];
        }
    } else {
        ecs_os_memset(e, 0, ECS_SIZEOF(ecs_entity_t) * to_add);
    }
    ecs_os_memset(r, 0, ECS_SIZEOF(ecs_record_t*) * to_add);

    /* Add elements to each column array */
    ecs_lifecycle_t *lc_array = data->lc;
    for (i = 0; i < column_count; i ++) {
        ecs_column_t *column = &columns[i];
        if (!column->size) {
            continue;
        }

        ecs_lifecycle_t *lc = NULL;
        if (lc_array) {
            lc = &lc_array[i];
        }

        grow_column(column, lc, to_add, size, true);
        ecs_assert(ecs_vector_size(columns[i].data) == size, 
            ECS_INTERNAL_ERROR);
    }

    /* If the table is monitored indicate that there has been a change */
    mark_dirty(data, 0);

    if (!world->in_progress && !cur_count) {
        // ecs_table_activate(world, table, 0, true);
    }

    data->alloc_count ++;

    /* Return index of first added entity */
    return cur_count;
}

static
void fast_append(
    ecs_column_t *columns,
    int32_t column_count)
{
    /* Add elements to each column array */
    int32_t i;
    for (i = 0; i < column_count; i ++) {
        ecs_column_t *column = &columns[i];
        int16_t size = column->size;
        if (size) {
            _ecs_vector_add(&column->data, size);
        }
    }
}

int32_t ecs_table_append(
    ecs_world_t * world,
    ecs_table_t * table,
    ecs_data_t * data,
    ecs_entity_t entity,
    ecs_record_t * record,
    bool construct)
{
    ecs_assert(table != NULL, ECS_INTERNAL_ERROR);
    ecs_assert(data != NULL, ECS_INTERNAL_ERROR);

    /* Get count & size before growing entities array. This tells us whether the
     * arrays will realloc */
    int32_t count = ecs_vector_count(data->entities);
    int32_t size = ecs_vector_size(data->entities);

    int32_t column_count = table->column_count;
    ecs_column_t *columns;
    ensure_data(world, table, data, &column_count, &columns);

    /* Grow buffer with entity ids, set new element to new entity */
    ecs_entity_t *e = ecs_vector_add(&data->entities, ecs_entity_t);
    ecs_assert(e != NULL, ECS_INTERNAL_ERROR);
    *e = entity;    

    /* Keep track of alloc count. This allows references to check if cached
     * pointers need to be updated. */  
    data->alloc_count += (count == size);

    /* Add record ptr to array with record ptrs */
    ecs_record_t **r = ecs_vector_add(&data->record_ptrs, ecs_record_t*);
    ecs_assert(r != NULL, ECS_INTERNAL_ERROR);
    *r = record;
 
    /* If the table is monitored indicate that there has been a change */
    mark_dirty(data, 0);

    /* If this is the first entity in this table, signal queries so that the
     * table moves from an inactive table to an active table. */
    if (!world->in_progress && !count) {
        // ecs_table_activate(world, table, 0, true);
    }

    ecs_assert(count >= 0, ECS_INTERNAL_ERROR);

    /* Fast path: no switch columns, no lifecycle actions */
    if (!(table->flags & EcsTableIsComplex)) {
        fast_append(columns, column_count);
        return count;
    }

    ecs_lifecycle_t *lc_array = data->lc;

    /* Reobtain size to ensure that the columns have the same size as the 
     * entities and record vectors. This keeps reasoning about when allocations
     * occur easier. */
    size = ecs_vector_size(data->entities);

    /* Grow component arrays with 1 element */
    int32_t i;
    for (i = 0; i < column_count; i ++) {
        ecs_column_t *column = &columns[i];
        if (!column->size) {
            continue;
        }

        ecs_lifecycle_t *lc = NULL;
        if (lc_array) {
            lc = &lc_array[i];
        }

        grow_column(column, lc, 1, size, construct);
        
        ecs_assert(
            ecs_vector_size(columns[i].data) == ecs_vector_size(data->entities), 
            ECS_INTERNAL_ERROR); 
            
        ecs_assert(
            ecs_vector_count(columns[i].data) == ecs_vector_count(data->entities), 
            ECS_INTERNAL_ERROR);                        
    }

    return count;
}

static
void fast_delete_last(
    ecs_column_t *columns,
    int32_t column_count) 
{
    int i;
    for (i = 0; i < column_count; i ++) {
        ecs_column_t *column = &columns[i];
        ecs_vector_remove_last(column->data);
    }
}

static
void fast_delete(
    ecs_column_t *columns,
    int32_t column_count,
    int32_t index) 
{
    int i;
    for (i = 0; i < column_count; i ++) {
        ecs_column_t *column = &columns[i];
        int16_t size = column->size;
        if (size) {
            _ecs_vector_remove(column->data, size, index);
        } 
    }
}

void ecs_table_delete(
    ecs_world_t * world,
    ecs_table_t * table,
    ecs_data_t * data,
    int32_t index,
    bool destruct)
{
    ecs_assert(world != NULL, ECS_INTERNAL_ERROR);
    ecs_assert(table != NULL, ECS_INTERNAL_ERROR);
    ecs_assert(data != NULL, ECS_INTERNAL_ERROR);

    ecs_vector_t *entity_column = data->entities;
    int32_t count = ecs_vector_count(entity_column);

    ecs_assert(count > 0, ECS_INTERNAL_ERROR);
    count --;
    
    ecs_assert(index <= count, ECS_INTERNAL_ERROR);

    ecs_lifecycle_t *lc_array = data->lc;
    int32_t column_count = table->column_count;
    int32_t i;

    ecs_entity_t *entities = ecs_vector_first(entity_column, ecs_entity_t);
    ecs_entity_t entity_to_move = entities[count];

    /* Move last entity id to index */
    entities[index] = entity_to_move;
    ecs_vector_remove_last(entity_column);

    /* Move last record ptr to index */
    ecs_vector_t *record_column = data->record_ptrs;     
    ecs_record_t **records = ecs_vector_first(record_column, ecs_record_t*);

    ecs_assert(count < ecs_vector_count(record_column), ECS_INTERNAL_ERROR);
    ecs_record_t *record_to_move = records[count];

    records[index] = record_to_move;
    ecs_vector_remove_last(record_column);    

    /* Update record of moved entity in entity index */
    if (index != count) {
        if (record_to_move) {
            if (record_to_move->row >= 0) {
                record_to_move->row = index + 1;
            } else {
                record_to_move->row = -(index + 1);
            }
            ecs_assert(record_to_move->table != NULL, ECS_INTERNAL_ERROR);
            ecs_assert(record_to_move->table == table, ECS_INTERNAL_ERROR);
        }
    } 

    /* If the table is monitored indicate that there has been a change */
    mark_dirty(data, 0);

    if (!count) {
        // ecs_table_activate(world, table, NULL, false);
    }

    /* Move each component value in array to index */
    ecs_column_t *columns = data->columns;

    if (!(table->flags & EcsTableIsComplex)) {
        if (index == count) {
            fast_delete_last(columns, column_count);
        } else {
            fast_delete(columns, column_count, index);
        }
        return;
    }

    for (i = 0; i < column_count; i ++) {
        ecs_column_t *column = &columns[i];
        int16_t size = column->size;
        if (size) {
            ecs_lifecycle_t *lc = lc_array ? &lc_array[i] : NULL;
            ecs_xtor_t dtor;

            void *dst = _ecs_vector_get(column->data, size, index);

            ecs_move_t move;
            if (lc && (count != index) && (move = lc->move)) {
                void *src = _ecs_vector_get(column->data, size, count);

                /* If the delete is not destructing the component, the component
                * was already deleted, most likely by a move. In that case we
                * still need to move, but we need to make sure we're moving
                * into an element that is initialized with valid memory, so
                * call the constructor. */
                if (!destruct) {
                    ecs_xtor_t ctor = lc->ctor;
                    ecs_assert(ctor != NULL, ECS_INTERNAL_ERROR);
                    ctor(dst, ecs_to_size_t(size), 1);   
                }

                /* Move last element into deleted element */
                move(dst, src, ecs_to_size_t(size), 1);

                /* Memory has been copied, we can now simply remove last */
                ecs_vector_remove_last(column->data);                              
            } else {
                if (destruct && lc && (dtor = lc->dtor)) {
                    dtor(dst, ecs_to_size_t(size), 1);
                }

                _ecs_vector_remove(column->data, size, index);
            }
        }
    }
}

static
void fast_move(
    ecs_table_t * new_table,
    ecs_data_t * new_data,
    int32_t new_index,
    ecs_table_t * old_table,
    ecs_data_t * old_data,
    int32_t old_index)
{
    ecs_type_t new_type = new_table->type;
    ecs_type_t old_type = old_table->type;

    int32_t i_new = 0, new_column_count = new_table->column_count;
    int32_t i_old = 0, old_column_count = old_table->column_count;
    ecs_entity_t *new_components = ecs_vector_first(new_type, ecs_entity_t);
    ecs_entity_t *old_components = ecs_vector_first(old_type, ecs_entity_t);

    ecs_column_t *old_columns = old_data->columns;
    ecs_column_t *new_columns = new_data->columns;

    for (; (i_new < new_column_count) && (i_old < old_column_count);) {
        ecs_entity_t new_component = new_components[i_new];
        ecs_entity_t old_component = old_components[i_old];

        if (new_component == old_component) {
            ecs_column_t *new_column = &new_columns[i_new];
            ecs_column_t *old_column = &old_columns[i_old];
            int16_t size = new_column->size;

            if (size) {
                void *dst = _ecs_vector_get(new_column->data, size, new_index);
                void *src = _ecs_vector_get(old_column->data, size, old_index);

                ecs_assert(dst != NULL, ECS_INTERNAL_ERROR);
                ecs_assert(src != NULL, ECS_INTERNAL_ERROR);
                ecs_os_memcpy(dst, src, size); 
            }
        }

        i_new += new_component <= old_component;
        i_old += new_component >= old_component;
    }
}

void ecs_table_move(
    ecs_world_t * world,
    ecs_entity_t dst_entity,
    ecs_entity_t src_entity,
    ecs_table_t *new_table,
    ecs_data_t *new_data,
    int32_t new_index,
    ecs_table_t *old_table,
    ecs_data_t *old_data,
    int32_t old_index)
{
    (void)world;
    
    ecs_assert(new_table != NULL, ECS_INTERNAL_ERROR);
    ecs_assert(old_table != NULL, ECS_INTERNAL_ERROR);

    ecs_assert(old_index >= 0, ECS_INTERNAL_ERROR);
    ecs_assert(new_index >= 0, ECS_INTERNAL_ERROR);

    ecs_assert(old_data != NULL, ECS_INTERNAL_ERROR);
    ecs_assert(new_data != NULL, ECS_INTERNAL_ERROR);

    if (!((new_table->flags | old_table->flags) & EcsTableIsComplex)) {
        fast_move(new_table, new_data, new_index, old_table, old_data, old_index);
        return;
    }

    bool same_entity = dst_entity == src_entity;

    ecs_type_t new_type = new_table->type;
    ecs_type_t old_type = old_table->type;

    int32_t i_new = 0, new_column_count = new_table->column_count;
    int32_t i_old = 0, old_column_count = old_table->column_count;
    ecs_entity_t *new_components = ecs_vector_first(new_type, ecs_entity_t);
    ecs_entity_t *old_components = ecs_vector_first(old_type, ecs_entity_t);

    ecs_column_t *old_columns = old_data->columns;
    ecs_column_t *new_columns = new_data->columns;

    for (; (i_new < new_column_count) && (i_old < old_column_count);) {
        ecs_entity_t new_component = new_components[i_new];
        ecs_entity_t old_component = old_components[i_old];

        if (new_component == old_component) {
            ecs_column_t *new_column = &new_columns[i_new];
            ecs_column_t *old_column = &old_columns[i_old];
            int16_t size = new_column->size;

            if (size) {
                void *dst = _ecs_vector_get(new_column->data, size, new_index);
                void *src = _ecs_vector_get(old_column->data, size, old_index);

                ecs_assert(dst != NULL, ECS_INTERNAL_ERROR);
                ecs_assert(src != NULL, ECS_INTERNAL_ERROR);

                ecs_lifecycle_t *lc = &new_data->lc[i_new];
                if (same_entity) {
                    ecs_move_t move;
                    if (lc && (move = lc->move)) {
                        ecs_xtor_t ctor = lc->ctor;

                        /* Ctor should always be set if copy is set */
                        ecs_assert(ctor != NULL, ECS_INTERNAL_ERROR);

                        /* Construct a new value, move the value to it */
                        ctor(dst, ecs_to_size_t(size), 1);
                        move(dst, src, ecs_to_size_t(size), 1);
                    } else {
                        ecs_os_memcpy(dst, src, size);
                    }
                } else {
                    ecs_copy_t copy;
                    if (lc && (copy = lc->copy)) {
                        ecs_xtor_t ctor = lc->ctor;

                        /* Ctor should always be set if copy is set */
                        ecs_assert(ctor != NULL, ECS_INTERNAL_ERROR);
                        ctor(dst, ecs_to_size_t(size), 1);
                        copy(dst, src, ecs_to_size_t(size), 1);
                    } else {
                        ecs_os_memcpy(dst, src, size);
                    }
                }
            }
        } else {
            if (new_component < old_component) {
                ctor_component(&new_data->lc[i_new],
                    &new_columns[i_new], new_index, 1);
            } else {
                dtor_component(&old_data->lc[i_old],
                    &old_columns[i_old], old_index, 1);
            }
        }

        i_new += new_component <= old_component;
        i_old += new_component >= old_component;
    }

    for (; (i_new < new_column_count); i_new ++) {
        ctor_component(&new_data->lc[i_new], &new_columns[i_new], new_index, 1);
    }

    for (; (i_old < old_column_count); i_old ++) {
        dtor_component(&old_data->lc[i_old], &old_columns[i_old], old_index, 1);
    }
}

int32_t ecs_table_appendn(
    ecs_world_t * world,
    ecs_table_t * table,
    ecs_data_t * data,
    int32_t to_add,
    const ecs_entity_t *ids)
{
    int32_t cur_count = ecs_table_data_count(data);
    return grow_data(world, table, data, to_add, cur_count + to_add, ids);
}

void ecs_table_set_size(
    ecs_world_t * world,
    ecs_table_t * table,
    ecs_data_t * data,
    int32_t size)
{
    int32_t cur_count = ecs_table_data_count(data);

    if (cur_count < size) {
        grow_data(world, table, data, 0, size, NULL);
    } else if (!size) {
        /* Initialize columns if 0 is passed. This is a shortcut to initialize
         * columns when, for example, an API call is inserting bulk data. */
        int32_t column_count = table->column_count;
        ecs_column_t *columns;
        ensure_data(world, table, data, &column_count, &columns);
    }
}

int32_t ecs_table_data_count(
    ecs_data_t *data)
{
    return data ? ecs_vector_count(data->entities) : 0;
}

int32_t ecs_table_count(
    ecs_table_t *table)
{
    ecs_assert(table != NULL, ECS_INTERNAL_ERROR);
    ecs_data_t *data = table->data;
    if (!data) {
        return 0;
    }

    return ecs_table_data_count(data);
}

int32_t* ecs_table_get_dirty_state(
    ecs_table_t *table)
{
    ecs_data_t *data = ecs_table_get_or_create_data(table);
    if (!data->dirty_state) {
        data->dirty_state = ecs_os_calloc(ECS_SIZEOF(int32_t) * (table->column_count + 1));
        ecs_assert(data->dirty_state != NULL, ECS_INTERNAL_ERROR);
    }
    return data->dirty_state;
}

int32_t* ecs_table_get_monitor(
    ecs_table_t *table)
{
    int32_t *dirty_state = ecs_table_get_dirty_state(table);
    ecs_assert(dirty_state != NULL, ECS_INTERNAL_ERROR);

    int32_t column_count = table->column_count;
    return ecs_os_memdup(dirty_state, (column_count + 1) * ECS_SIZEOF(int32_t));
}

void ecs_table_notify(
    ecs_world_t * world,
    ecs_table_t * table,
    ecs_table_event_t * event)
{
    switch(event->kind) {
    case EcsTableComponentInfo:
        notify_component_info(world, table, event->component);
        break;
    }
}

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


static
void* get_component_w_index(
    ecs_entity_info_t *info,
    int32_t index)
{
    ecs_data_t *data = info->data;
    ecs_assert(data != NULL, ECS_INTERNAL_ERROR);

    ecs_column_t *columns = data->columns;
    ecs_assert(columns != NULL, ECS_INTERNAL_ERROR);
    ecs_assert(index < info->table->column_count, ECS_INVALID_COMPONENT);

    ecs_column_t *column = &columns[index];
    ecs_vector_t *data_vec = column->data;
    int16_t size = column->size; 
    
    /* If size is 0, component does not have a value. This is likely caused by
     * an application trying to call ecs_get with a tag. */
    ecs_assert(size != 0, ECS_INVALID_PARAMETER);

    /* This function should not be called if an entity does not exist in the
     * provided table. Therefore if the component is found in the table, and an
     * entity exists for it, the vector cannot be NULL */
    ecs_assert(data_vec != NULL, ECS_INTERNAL_ERROR);

    void *ptr = _ecs_vector_first(data_vec, size);

    /* This could only happen when the vector is empty, which should not be
     * possible since the vector should at least have one element */
    ecs_assert(ptr != NULL, ECS_INTERNAL_ERROR);

    return ECS_OFFSET(ptr, info->row * size);
}

/* Get pointer to single component value */
static
void* get_component(
    ecs_entity_info_t *info,
    ecs_entity_t component)
{
    ecs_assert(info->table != NULL, ECS_INTERNAL_ERROR);
    ecs_assert(component != 0, ECS_INTERNAL_ERROR);
    ecs_assert(info->row >= 0, ECS_INTERNAL_ERROR);

    ecs_table_t *table = info->table;
    ecs_type_t type = table->type;

    ecs_entity_t *ids = ecs_vector_first(type, ecs_entity_t);

    /* The table column_count contains the maximum column index that actually
     * contains data. This excludes component ids that do not have data, such
     * as tags. Therefore it is faster to iterate column_count vs. all the
     * elements in the type.
     *
     * The downside of this is that the code can't always detect when an 
     * application attempts to get the value of a tag (which is not allowed). To
     * ensure consistent behavior in debug mode, the entire type is iterated as
     * this guarantees that the code will assert when attempting to obtain the
     * value of a tag. */
#ifndef NDEBUG
    int i, count = ecs_vector_count(type);
#else
    int i, count = table->column_count;
#endif

    for (i = 0; i < count; i ++) {
        if (ids[i] == component) {
            return get_component_w_index(info, i);
        }
    }

    return NULL;
}

/* Utility to compute actual row from row in record */
static
int32_t set_row_info(
    ecs_entity_info_t *info,
    int32_t row)
{
    return info->row = ecs_record_to_row(row, &info->is_watched);
}

/* Utility to set info from main stage record */
static
void set_info_from_record(
    ecs_entity_t e,
    ecs_entity_info_t *info,
    ecs_record_t *record)
{
    (void)e;
    
    ecs_assert(record != NULL, ECS_INTERNAL_ERROR);

    info->record = record;

    ecs_table_t *table = record->table;

    set_row_info(info, record->row);

    info->table = table;
    if (!info->table) {
        info->data = NULL;
        return;
    }

    ecs_data_t *data = ecs_table_get_data(table);
    info->data = data;

    ecs_assert(!data || ecs_vector_count(data->entities) > info->row, 
        ECS_INTERNAL_ERROR);
}

/* Get info from main stage */
bool ecs_get_info(
    const ecs_world_t *world,
    ecs_entity_t entity,
    ecs_entity_info_t *info)
{
    ecs_record_t *record = ecs_eis_get(world, entity);

    if (!record) {
        info->table = NULL;
        info->is_watched = false;
        info->record = NULL;
        return false;
    }

    set_info_from_record(entity, info, record);

    return true;
}

static
ecs_lifecycle_t *get_lifecycle(
    ecs_world_t *world,
    ecs_entity_t component)
{
    ecs_entity_t real_id = ecs_get_type_id_from_id(world, component);
    if (real_id) {
        return ecs_get_lifecycle(world, real_id);
    } else {
        return NULL;
    }
}

static
int32_t new_entity(
    ecs_world_t * world,
    ecs_entity_t entity,
    ecs_entity_info_t * info,
    ecs_table_t * new_table)
{
    ecs_record_t *record = info->record;
    ecs_data_t *new_data = ecs_table_get_or_create_data(new_table);
    int32_t new_row;

    if (!record) {
        record = ecs_eis_ensure(world, entity);
    }

    new_row = ecs_table_append(
        world, new_table, new_data, entity, record, true);

    record->table = new_table;
    record->row = ecs_row_to_record(new_row, info->is_watched);

    ecs_assert(
        ecs_vector_count(new_data[0].entities) > new_row, 
        ECS_INTERNAL_ERROR);

    info->data = new_data;
    
    return new_row;
}

static
int32_t move_entity(
    ecs_world_t * world,
    ecs_entity_t entity,
    ecs_entity_info_t * info,
    ecs_table_t * src_table,
    ecs_data_t * src_data,
    int32_t src_row,
    ecs_table_t * dst_table)
{    
    ecs_data_t *dst_data = ecs_table_get_or_create_data(dst_table);
    ecs_assert(src_data != dst_data, ECS_INTERNAL_ERROR);
    ecs_assert(ecs_is_alive(world, entity), ECS_INVALID_PARAMETER);
    ecs_assert(src_table != NULL, ECS_INTERNAL_ERROR);
    ecs_assert(src_data != NULL, ECS_INTERNAL_ERROR);
    ecs_assert(src_row >= 0, ECS_INTERNAL_ERROR);
    ecs_assert(ecs_vector_count(src_data->entities) > src_row, 
        ECS_INTERNAL_ERROR);

    ecs_record_t *record = info->record;
    ecs_assert(!record || record == ecs_eis_get(world, entity), 
        ECS_INTERNAL_ERROR);

    int32_t dst_row = ecs_table_append(world, dst_table, dst_data, entity, 
        record, false);

    record->table = dst_table;
    record->row = ecs_row_to_record(dst_row, info->is_watched);

    ecs_assert(ecs_vector_count(src_data->entities) > src_row, 
        ECS_INTERNAL_ERROR);

    /* Copy entity & components from src_table to dst_table */
    if (src_table->type) {
        ecs_table_move(world, entity, entity, dst_table, dst_data, dst_row, 
            src_table, src_data, src_row);          
    }
    
    ecs_table_delete(world, src_table, src_data, src_row, false);

    info->data = dst_data;

    return dst_row;
}

static
void delete_entity(
    ecs_world_t * world,
    ecs_table_t * src_table,
    ecs_data_t * src_data,
    int32_t src_row)
{
    ecs_table_delete(world, src_table, src_data, src_row, true);
}

static
void commit(
    ecs_world_t * world,
    ecs_entity_t entity,
    ecs_entity_info_t * info,
    ecs_table_t * dst_table)
{
    ecs_assert(!world->in_progress, ECS_INTERNAL_ERROR);
    
    ecs_table_t *src_table = info->table;
    if (src_table == dst_table) {
        return;
    }

    if (src_table) {
        ecs_data_t *src_data = info->data;
        ecs_assert(dst_table != NULL, ECS_INTERNAL_ERROR);

        if (dst_table) { 
            info->row = move_entity(world, entity, info, src_table, 
                src_data, info->row, dst_table);
            info->table = dst_table;
        } else {
            delete_entity(world, src_table, src_data, info->row);
            ecs_record_t *r = ecs_eis_ensure(world, entity);
            r->table = NULL;
            r->row = (info->is_watched == true) * -1;
        }      
    } else {        
        if (dst_table) {
            info->row = new_entity(world, entity, info, dst_table);
            info->table = dst_table;
        }        
    } 
}

static
void add_w_info(
    ecs_world_t * world,
    ecs_entity_t entity,
    ecs_entity_info_t * info,
    ecs_entity_t component)
{
    ecs_table_t *dst_table = ecs_table_traverse_add(
        world, info->table, component);

    commit(world, entity, info, dst_table);
}

static
void *get_mutable(
    ecs_world_t * world,
    ecs_entity_t entity,
    ecs_entity_t component,
    ecs_entity_info_t * info)
{
    ecs_assert(world != NULL, ECS_INVALID_PARAMETER);
    ecs_assert(component != 0, ECS_INVALID_PARAMETER);

    ecs_store_t *store = ecs_store_ensure(world, component);
    ecs_dense_t *sparse = store->sparse;
    if (sparse) {
        struct ecs_dense_ensure_get_t ret = _ecs_dense_ensure_get(
            sparse, 0, entity);
        if (ret.added) {
            ecs_record_t *r = ecs_eis_ensure(world, entity);
            r->table = ecs_table_traverse_add(
                world, r->table, ECS_TAG | component);
        }
        return ret.ptr;
    }

    void *dst = NULL;
    if (ecs_get_info(world, entity, info) && info->table) {
        dst = get_component(info, component);
    }

    if (!dst) {
        add_w_info(world, entity, info, component);
        ecs_get_info(world, entity, info);
        ecs_assert(info->table != NULL, ECS_INTERNAL_ERROR);
        return get_component(info, component);
    } else {
        return dst;
    }
}

int32_t ecs_record_to_row(
    int32_t row, 
    bool *is_watched_out) 
{
    bool is_watched = row < 0;
    row = row * -(is_watched * 2 - 1) - 1 * (row != 0);
    *is_watched_out = is_watched;
    return row;
}

int32_t ecs_row_to_record(
    int32_t row, 
    bool is_watched) 
{
    return (row + 1) * -(is_watched * 2 - 1);
}

ecs_entity_t ecs_new_id(
    ecs_world_t *world)
{
    ecs_entity_t entity;

    entity = ecs_eis_recycle(world);

    return entity;
}

ecs_entity_t ecs_new_component_id(
    ecs_world_t *world)
{    
    if (world->stats.last_component_id >= ECS_HI_COMPONENT_ID) {
        /* If the low component ids are depleted, return a regular entity id */
        return ecs_new_id(world);
    } else {
        return world->stats.last_component_id ++;
    }
}

void ecs_add_id(
    ecs_world_t *world,
    ecs_entity_t entity,
    ecs_entity_t component)
{
    ecs_assert(world != NULL, ECS_INVALID_PARAMETER);
    ecs_assert(component != 0, ECS_INVALID_PARAMETER);
    ecs_stage_t *stage = ecs_get_stage(&world);

    if (ecs_defer_add(world, stage, entity, component)) {
        return;
    }

    ecs_store_t *store = ecs_store_ensure(world, component);
    ecs_dense_t *sparse = store->sparse;
    if (sparse) {
        if (_ecs_dense_ensure(sparse, 0, entity)) {
            ecs_record_t *r = ecs_eis_ensure(world, entity);
            r->table = ecs_table_traverse_add(
                world, r->table, ECS_TAG | component);
        }
        ecs_defer_flush(world, stage);
        return;
    }

    ecs_entity_info_t info;
    ecs_get_info(world, entity, &info);

    ecs_table_t *src_table = info.table;
    ecs_table_t *dst_table = ecs_table_traverse_add(
        world, src_table, component);

    commit(world, entity, &info, dst_table);

    ecs_defer_flush(world, stage);
}

void ecs_remove_id(
    ecs_world_t *world,
    ecs_entity_t entity,
    ecs_entity_t component)
{
    ecs_assert(world != NULL, ECS_INVALID_PARAMETER);
    ecs_assert(component != 0, ECS_INVALID_PARAMETER); 
    ecs_stage_t *stage = ecs_get_stage(&world);

    if (ecs_defer_remove(world, stage, entity, component)) {
        return;
    }

    ecs_store_t *store = ecs_store_ensure(world, component);
    ecs_dense_t *sparse = store->sparse;
    if (sparse) {
        if (ecs_dense_remove(sparse, entity)) {
            ecs_record_t *r = ecs_eis_ensure(world, entity);
            r->table = ecs_table_traverse_remove(
                world, r->table, ECS_TAG | component);
        }
        ecs_defer_flush(world, stage);
        return;
    }

    ecs_entity_info_t info;
    ecs_get_info(world, entity, &info);

    ecs_table_t *src_table = info.table;
    ecs_table_t *dst_table = ecs_table_traverse_remove(
        world, src_table, component);

    commit(world, entity, &info, dst_table);

    ecs_defer_flush(world, stage);
}

void ecs_clear(
    ecs_world_t *world,
    ecs_entity_t entity)
{
    ecs_assert(world != NULL, ECS_INVALID_PARAMETER);
    ecs_assert(entity != 0, ECS_INVALID_PARAMETER);

    ecs_stage_t *stage = ecs_get_stage(&world);
    if (ecs_defer_clear(world, stage, entity)) {
        return;
    }

    ecs_entity_info_t info;
    info.table = NULL;
    ecs_get_info(world, entity, &info);
    commit(world, entity, &info, NULL);

    ecs_defer_flush(world, stage);
}

void ecs_delete(
    ecs_world_t *world,
    ecs_entity_t entity)
{
    ecs_assert(world != NULL, ECS_INVALID_PARAMETER);
    ecs_assert(entity != 0, ECS_INVALID_PARAMETER);

    ecs_stage_t *stage = ecs_get_stage(&world);
    if (ecs_defer_delete(world, stage, entity)) {
        return;
    }

    ecs_record_t *r = ecs_sparse_remove_get(
        world->entity_index, ecs_record_t, entity);

    if (r) {
        ecs_entity_info_t info;
        set_info_from_record(entity, &info, r);

        if (info.data) {
            /* Table contains data, remove entity from table */
            delete_entity(world, info.table, info.data, info.row);
            r->table = NULL;
        } else if (info.table) {
            /* Table contains no data, which means it is used to keep track of
             * sparse components. Iterate type & remove from sparse sets */
            ecs_type_t type = info.table->type;
            int32_t i, count = ecs_vector_count(type);
            ecs_entity_t *entities = ecs_vector_first(type, ecs_entity_t);

            for (i = 0; i < count; i ++) {
                ecs_entity_t e = entities[i] & ~ECS_FLAGS;
                ecs_store_t *store = 
                    ecs_ptree_get(&world->stores, ecs_store_t, e);
                ecs_dense_t *sparse;
                if (store && (sparse = store->sparse)) {
                    ecs_dense_remove(sparse, entity);
                }
            }
        }

        r->row = 0;
    }

    ecs_defer_flush(world, stage);
}

const void* ecs_get_id(
    const ecs_world_t *world,
    ecs_entity_t entity,
    ecs_entity_t component)
{
    ecs_assert(world != NULL, ECS_INVALID_PARAMETER);
    ecs_assert(world->magic == ECS_WORLD_MAGIC, ECS_INTERNAL_ERROR);

    ecs_store_t *store = ecs_store_get(world, component);
    if (!store) {
        return NULL;
    }

    ecs_dense_t *sparse = store->sparse;
    if (sparse) {
        return _ecs_dense_get(sparse, 0, entity);
    }

    ecs_entity_info_t info;
    void *ptr = NULL;
    bool found = ecs_get_info(world, entity, &info);
    if (found) {
        if (!info.table) {
            return NULL;
        }

        ptr = get_component(&info, component);        
    }

    return ptr;
}

void* ecs_get_mut_id(
    ecs_world_t *world,
    ecs_entity_t entity,
    ecs_entity_t component)
{
    ecs_stage_t *stage = ecs_get_stage(&world);
    void *result;

    if (ecs_defer_set(
        world, stage, EcsOpMut, entity, component, 0, NULL, &result))
    {
        return result;
    }

    ecs_entity_info_t info;
    result = get_mutable(world, entity, component, &info);
    
    ecs_defer_flush(world, stage);

    return result;
}

void ecs_modified_id(
    ecs_world_t *world,
    ecs_entity_t entity,
    ecs_entity_t component)
{
    ecs_stage_t *stage = ecs_get_stage(&world);

    if (ecs_defer_modified(world, stage, entity, component)) {
        return;
    }
    
    ecs_defer_flush(world, stage);
}

static
ecs_entity_t assign_ptr_w_id(
    ecs_world_t *world,
    ecs_entity_t entity,
    ecs_entity_t component,
    size_t size,
    void * ptr,
    bool is_move,
    bool notify)
{
    ecs_stage_t *stage = ecs_get_stage(&world);
    (void)notify;

    if (!entity) {
        entity = ecs_new_id(world);
    }

    if (ecs_defer_set(world, stage, EcsOpSet, entity, component, 
        ecs_from_size_t(size), ptr, NULL))
    {
        return entity;
    }

    ecs_entity_info_t info;

    void *dst = get_mutable(world, entity, component, &info);

    ecs_assert(dst != NULL, ECS_INTERNAL_ERROR);

    if (ptr) {
        ecs_entity_t real_id = ecs_get_type_id_from_id(world, component);
        ecs_lifecycle_t *cdata = get_lifecycle(world, real_id);
        if (cdata) {
            if (is_move) {
                ecs_move_t move = cdata->move;
                if (move) {
                    move(dst, ptr, size, 1);
                } else {
                    ecs_os_memcpy(dst, ptr, ecs_from_size_t(size));
                }
            } else {
                ecs_copy_t copy = cdata->copy;
                if (copy) {
                    copy(dst, ptr, size, 1);
                } else {
                    ecs_os_memcpy(dst, ptr, ecs_from_size_t(size));
                }
            }
        } else {
            ecs_os_memcpy(dst, ptr, ecs_from_size_t(size));
        }
    } else {
        memset(dst, 0, size);
    }

    ecs_defer_flush(world, stage);

    return entity;
}

ecs_entity_t ecs_set_ptr_id(
    ecs_world_t *world,
    ecs_entity_t entity,
    ecs_entity_t component,
    const void *ptr,
    size_t size)
{
    /* Safe to cast away const: function won't modify if move arg is false */
    return assign_ptr_w_id(
        world, entity, component, size, (void*)ptr, false, true);
}

bool ecs_has_id(
    const ecs_world_t *world,
    ecs_entity_t entity,
    ecs_entity_t component)
{
    ecs_type_t type = ecs_get_type(world, entity);
    return ecs_type_has_entity(world, type, component);
}

const char* ecs_get_name(
    const ecs_world_t *world,
    ecs_entity_t entity)
{
    const EcsName *id = ecs_get(world, entity, EcsName);

    if (id) {
        return id->value;
    } else {
        return NULL;
    }
}

bool ecs_is_alive(
    const ecs_world_t *world,
    ecs_entity_t e)
{
    return ecs_eis_is_alive(world, e);
}

bool ecs_exists(
    const ecs_world_t *world,
    ecs_entity_t e)
{
    return ecs_eis_exists(world, e);
}

ecs_type_t ecs_get_type(
    const ecs_world_t *world,
    ecs_entity_t entity)
{
    ecs_assert(world != NULL, ECS_INVALID_PARAMETER);
    ecs_record_t *record = NULL;

    record = ecs_eis_get(world, entity);

    ecs_table_t *table;
    if (record && (table = record->table)) {
        return table->type;
    }
    
    return NULL;
}

void ecs_defer_begin(
    ecs_world_t *world)
{
    ecs_stage_t *stage = ecs_get_stage(&world);
    
    if (world->in_progress) {
        ecs_stage_defer_begin(world, stage);
    } else {
        ecs_defer_none(world, stage);
    }
}

void ecs_defer_end(
    ecs_world_t *world)
{
    ecs_stage_t *stage = ecs_get_stage(&world);
    
    if (world->in_progress) {
        ecs_stage_defer_end(world, stage);
    } else {
        ecs_defer_flush(world, stage);
    }
}

static
void discard_op(
    ecs_defer_t * op)
{
    void *value = op->is._1.value;
    if (value) {
        ecs_os_free(value);
    }

    ecs_entity_t *components = op->components.array;
    if (components) {
        ecs_os_free(components);
    }
}

/* Leave safe section. Run all deferred commands. */
void ecs_defer_flush(
    ecs_world_t *world,
    ecs_stage_t *stage)
{
    if (!--stage->defer) {
        ecs_vector_t *defer_queue = stage->defer_queue;
        stage->defer_queue = NULL;
        if (defer_queue) {
            ecs_defer_t *ops = ecs_vector_first(defer_queue, ecs_defer_t);
            int32_t i, count = ecs_vector_count(defer_queue);
            
            for (i = 0; i < count; i ++) {
                ecs_defer_t *op = &ops[i];
                ecs_entity_t e = op->is._1.entity;

                /* If entity is no longer alive, this could be because the queue
                 * contained both a delete and a subsequent add/remove/set which
                 * should be ignored. */
                if (e && !ecs_is_alive(world, e) && ecs_eis_exists(world, e)) {
                    ecs_assert(op->kind != EcsOpNew, ECS_INTERNAL_ERROR);
                    discard_op(op);
                    continue;
                }

                if (op->components.count == 1) {
                    op->components.array = &op->component;
                }

                switch(op->kind) {
                case EcsOpNew:

                    /* Fallthrough */
                case EcsOpAdd:
                    ecs_add_id(world, e, op->component);
                    break;
                case EcsOpRemove:
                    ecs_remove_id(world, e, op->component);
                    break;
                case EcsOpSet:
                    assign_ptr_w_id(world, e, 
                        op->component, ecs_to_size_t(op->is._1.size), 
                        op->is._1.value, true, true);
                    break;
                case EcsOpMut:
                    assign_ptr_w_id(world, e, 
                        op->component, ecs_to_size_t(op->is._1.size), 
                        op->is._1.value, true, false);
                    break;
                case EcsOpModified:
                    ecs_modified_id(world, e, op->component);
                    break;
                case EcsOpDelete: {
                    ecs_delete(world, e);
                    break;
                }
                case EcsOpClear:
                    ecs_clear(world, e);
                    break;
                }

                if (op->components.count > 1) {
                    ecs_os_free(op->components.array);
                }

                if (op->is._1.value) {
                    ecs_os_free(op->is._1.value);
                }
            };

            if (defer_queue != stage->defer_merge_queue) {
                ecs_vector_free(defer_queue);
            }
        }
    }
}

static
size_t append_to_str(
    char **buffer,
    const char *str,
    size_t bytes_left,
    size_t *required)
{
    char *ptr = *buffer;

    size_t len = strlen(str);
    size_t to_write;
    if (bytes_left < len) {
        to_write = bytes_left;
        bytes_left = 0;
    } else {
        to_write = len;
        bytes_left -= len;
    }
    
    if (to_write) {
        ecs_os_memcpy(ptr, str, to_write);
    }

    (*required) += len;
    (*buffer) += to_write;

    return bytes_left;
}

static
size_t append_entity_to_str(
    ecs_world_t *world,
    char **ptr,
    ecs_entity_t entity,
    size_t bytes_left,
    size_t *required)
{
    const char *name = ecs_get_name(world, entity);
    if (name) {
        bytes_left = append_to_str(
            ptr, name, bytes_left, required);
    } else {
        char buf[14];
        ecs_os_sprintf(buf, "%u", (uint32_t)entity);
        bytes_left = append_to_str(
            ptr, buf, bytes_left, required);
    }
    return bytes_left;
}

size_t ecs_entity_str(
    ecs_world_t *world,
    ecs_entity_t entity,
    char *buffer,
    size_t buffer_len)
{
    char *ptr = buffer;
    size_t bytes_left = buffer_len - 1, required = 0;
    if (ecs_has_flag(entity, ECS_PAIR)) {
        bytes_left = append_entity_to_str(world, &ptr, ecs_get_role(entity), 
            bytes_left, &required);
        bytes_left = append_to_str(&ptr, "|", bytes_left, &required);
        entity = entity & ECS_ENTITY_MASK;
    }

    bytes_left = append_entity_to_str(world, &ptr, entity, bytes_left, 
        &required);

    ptr[0] = '\0';
    return required;
}

ecs_entity_t ecs_find_in_type(
    ecs_world_t *world,
    ecs_type_t type,
    ecs_entity_t component,
    ecs_entity_t role)
{
    ecs_vector_each(type, ecs_entity_t, c_ptr, {
        ecs_entity_t c = *c_ptr;

        if (role) {
            if (!ecs_has_role(c, role)) {
                continue;
            }
        }

        ecs_entity_t e = c & ECS_ENTITY_MASK;

        if (component) {
           ecs_type_t component_type = ecs_get_type(world, e);
           if (!ecs_type_has_entity(world, component_type, component)) {
               continue;
           }
        }

        return e;
    });

    return 0;
}

static
ecs_defer_t* new_defer_op(ecs_stage_t *stage) {
    ecs_defer_t *result = ecs_vector_add(&stage->defer_queue, ecs_defer_t);
    ecs_os_memset(result, 0, ECS_SIZEOF(ecs_defer_t));
    return result;
}

static 
void new_defer_component_ids(
    ecs_defer_t *op, 
    ecs_entities_t *components)
{
    int32_t components_count = components->count;
    if (components_count == 1) {
        ecs_entity_t component = components->array[0];
        op->component = component;
        op->components = (ecs_entities_t) {
            .array = NULL,
            .count = 1
        };
    } else if (components_count) {
        ecs_size_t array_size = components_count * ECS_SIZEOF(ecs_entity_t);
        op->components.array = ecs_os_malloc(array_size);
        ecs_os_memcpy(op->components.array, components->array, array_size);
        op->components.count = components_count;
    } else {
        op->component = 0;
        op->components = (ecs_entities_t){ 0 };
    }
}

static
bool defer_new(
    ecs_stage_t *stage,
    ecs_defer_kind_t op_kind,
    ecs_entity_t entity,
    ecs_entities_t *components)
{
    if (stage->defer) {
        ecs_entity_t scope = stage->scope;
        if (components) {
            if (!components->count && !scope) {
                return true;
            }
        }

        ecs_defer_t *op = new_defer_op(stage);
        op->kind = op_kind;
        op->scope = scope;
        op->is._1.entity = entity;

        new_defer_component_ids(op, components);

        return true;
    } else {
        stage->defer ++;
    }
    
    return false;
}

static
bool defer_add_remove(
    ecs_stage_t *stage,
    ecs_defer_kind_t op_kind,
    ecs_entity_t entity,
    ecs_entity_t component)
{
    if (stage->defer) {
        ecs_entity_t scope = stage->scope;

        ecs_defer_t *op = new_defer_op(stage);
        op->kind = op_kind;
        op->scope = scope;
        op->is._1.entity = entity;
        op->component = component;

        return true;
    } else {
        stage->defer ++;
    }
    
    return false;
}

bool ecs_defer_none(
    ecs_world_t *world,
    ecs_stage_t *stage)
{
    (void)world;
    stage->defer ++;
    return false;
}

bool ecs_defer_modified(
    ecs_world_t *world,
    ecs_stage_t *stage,
    ecs_entity_t entity,
    ecs_entity_t component)
{
    (void)world;
    if (stage->defer) {
        ecs_defer_t *op = new_defer_op(stage);
        op->kind = EcsOpModified;
        op->component = component;
        op->is._1.entity = entity;
        return true;
    } else {
        stage->defer ++;
    }
    
    return false;
}

bool ecs_defer_delete(
    ecs_world_t *world,
    ecs_stage_t *stage,
    ecs_entity_t entity)
{
    (void)world;
    if (stage->defer) {
        ecs_defer_t *op = new_defer_op(stage);
        op->kind = EcsOpDelete;
        op->is._1.entity = entity;
        return true;
    } else {
        stage->defer ++;
    }
    return false;
}

bool ecs_defer_clear(
    ecs_world_t *world,
    ecs_stage_t *stage,
    ecs_entity_t entity)
{
    (void)world;
    if (stage->defer) {
        ecs_defer_t *op = new_defer_op(stage);
        op->kind = EcsOpClear;
        op->is._1.entity = entity;
        return true;
    } else {
        stage->defer ++;
    }
    return false;
}

bool ecs_defer_new(
    ecs_world_t *world,
    ecs_stage_t *stage,
    ecs_entity_t entity,
    ecs_entities_t *components)
{   
    (void)world;
    return defer_new(stage, EcsOpNew, entity, components);
}

bool ecs_defer_add(
    ecs_world_t *world,
    ecs_stage_t *stage,
    ecs_entity_t entity,
    ecs_entity_t component)
{   
    (void)world;
    return defer_add_remove(stage, EcsOpAdd, entity, component);
}

bool ecs_defer_remove(
    ecs_world_t *world,
    ecs_stage_t *stage,
    ecs_entity_t entity,
    ecs_entity_t component)
{
    (void)world;
    return defer_add_remove(stage, EcsOpRemove, entity, component);
}

bool ecs_defer_set(
    ecs_world_t *world,
    ecs_stage_t *stage,
    ecs_defer_kind_t op_kind,
    ecs_entity_t entity,
    ecs_entity_t component,
    ecs_size_t size,
    const void *value,
    void **value_out)
{
    if (stage->defer) {
        if (!size) {
            const EcsType *cptr = ecs_get(world, component, EcsType);
            ecs_assert(cptr != NULL, ECS_INVALID_PARAMETER);
            size = cptr->size;
        }

        ecs_defer_t *op = new_defer_op(stage);
        op->kind = op_kind;
        op->component = component;
        op->is._1.entity = entity;
        op->is._1.size = size;
        op->is._1.value = ecs_os_malloc(size);

        if (!value) {
            value = ecs_get_id(world, entity, component);
        }

        ecs_lifecycle_t *lc = NULL;
        ecs_entity_t real_id = ecs_get_type_id_from_id(world, component);
        if (real_id) {
            lc = ecs_get_lifecycle(world, real_id);
        }
        ecs_xtor_t ctor;
        if (lc && (ctor = lc->ctor)) {
            ctor(op->is._1.value, ecs_to_size_t(size), 1);

            ecs_copy_t copy;
            if (value) {
                if ((copy = lc->copy)) {
                    copy(op->is._1.value, value, ecs_to_size_t(size), 1);
                } else {
                    ecs_os_memcpy(op->is._1.value, value, size);
                }
            }
        } else if (value) {
            ecs_os_memcpy(op->is._1.value, value, size);
        }
        
        if (value_out) {
            *value_out = op->is._1.value;
        }

        return true;
    } else {
        stage->defer ++;
    }
    
    return false;
}

void ecs_stage_merge(
    ecs_world_t *world,
    ecs_stage_t *stage)
{
    ecs_assert(stage != &world->stage, ECS_INTERNAL_ERROR);

    ecs_assert(stage->defer == 0, ECS_INVALID_PARAMETER);
    if (ecs_vector_count(stage->defer_merge_queue)) {
        stage->defer ++;
        stage->defer_queue = stage->defer_merge_queue;
        ecs_defer_flush(world, stage);
        ecs_vector_clear(stage->defer_merge_queue);
        ecs_assert(stage->defer_queue == NULL, ECS_INVALID_PARAMETER);
    }    
}

void ecs_stage_defer_begin(
    ecs_world_t *world,
    ecs_stage_t *stage)
{   
    (void)world; 
    ecs_defer_none(world, stage);
    if (stage->defer == 1) {
        stage->defer_queue = stage->defer_merge_queue;
    }
}

void ecs_stage_defer_end(
    ecs_world_t *world,
    ecs_stage_t *stage)
{ 
    (void)world;
    stage->defer --;
    if (!stage->defer) {
        stage->defer_merge_queue = stage->defer_queue;
        stage->defer_queue = NULL;
    }
}

void ecs_stage_init(
    ecs_world_t *world,
    ecs_stage_t *stage)
{
    memset(stage, 0, sizeof(ecs_stage_t));
    if (stage == &world->stage) {
        stage->id = 0;
    }
}

void ecs_stage_deinit(
    ecs_world_t *world,
    ecs_stage_t *stage)
{
    (void)world;
    ecs_vector_free(stage->defer_queue);
    ecs_vector_free(stage->defer_merge_queue);
}


struct ecs_vector_t {
    int32_t count;
    int32_t size;
#ifndef NDEBUG
    int64_t elem_size; /* 64 bit, so offset remains aligned to 8 bytes */
#endif
};

/** Resize the vector buffer */
static
ecs_vector_t* resize(
    ecs_vector_t *vector,
    int32_t size)
{
    ecs_vector_t *result = 
        ecs_os_realloc(vector, ECS_SIZEOF(ecs_vector_t) + size);
    ecs_assert(result != NULL, ECS_OUT_OF_MEMORY);
    return result;
}

/* -- Public functions -- */

ecs_vector_t* _ecs_vector_new(
    ecs_size_t elem_size,
    int32_t elem_count)
{
    ecs_assert(elem_size != 0, ECS_INTERNAL_ERROR);
    
    ecs_vector_t *result =
        ecs_os_malloc(ECS_SIZEOF(ecs_vector_t) + elem_size * elem_count);
    ecs_assert(result != NULL, ECS_OUT_OF_MEMORY);

    result->count = 0;
    result->size = elem_count;
#ifndef NDEBUG
    result->elem_size = elem_size;
#endif
    return result;
}

ecs_vector_t* _ecs_vector_from_array(
    ecs_size_t elem_size,
    int32_t elem_count,
    void *array)
{
    ecs_assert(elem_size != 0, ECS_INTERNAL_ERROR);
    
    ecs_vector_t *result =
        ecs_os_malloc(ECS_SIZEOF(ecs_vector_t) + elem_size * elem_count);
    ecs_assert(result != NULL, ECS_OUT_OF_MEMORY);

    ecs_os_memcpy(ECS_OFFSET(result, ECS_SIZEOF(ecs_vector_t)), array, 
        elem_size * elem_count);

    result->count = elem_count;
    result->size = elem_count;
#ifndef NDEBUG
    result->elem_size = elem_size;
#endif
    return result;   
}

void ecs_vector_free(
    ecs_vector_t *vector)
{
    ecs_os_free(vector);
}

void ecs_vector_clear(
    ecs_vector_t *vector)
{
    if (vector) {
        vector->count = 0;
    }
}

void ecs_vector_assert_size(
    ecs_vector_t *vector,
    ecs_size_t elem_size)
{
    if (vector) {
        ecs_assert(vector->elem_size == elem_size, ECS_INTERNAL_ERROR);
    }
}

void* _ecs_vector_addn(
    ecs_vector_t **array_inout,
    ecs_size_t elem_size,
    int32_t elem_count)
{
    ecs_assert(array_inout != NULL, ECS_INTERNAL_ERROR);
    
    if (elem_count == 1) {
        return _ecs_vector_add(array_inout, elem_size);
    }
    
    ecs_vector_t *vector = *array_inout;
    if (!vector) {
        vector = _ecs_vector_new(elem_size, 1);
        *array_inout = vector;
    }

    ecs_assert(vector->elem_size == elem_size, ECS_INTERNAL_ERROR);

    int32_t max_count = vector->size;
    int32_t old_count = vector->count;
    int32_t new_count = old_count + elem_count;

    if ((new_count - 1) >= max_count) {
        if (!max_count) {
            max_count = elem_count;
        } else {
            while (max_count < new_count) {
                max_count *= 2;
            }
        }

        vector = resize(vector, max_count * elem_size);
        vector->size = max_count;
        *array_inout = vector;
    }

    vector->count = new_count;

    return ECS_OFFSET(vector, ECS_SIZEOF(ecs_vector_t) + elem_size * old_count);
}

void* _ecs_vector_add(
    ecs_vector_t **array_inout,
    ecs_size_t elem_size)
{
    ecs_vector_t *vector = *array_inout;
    int32_t count, size;

    if (vector) {
        ecs_assert(vector->elem_size == elem_size, ECS_INTERNAL_ERROR);
        count = vector->count;
        size = vector->size;

        if (count >= size) {
            size = size * 2 + (size == 0) * 2;
            vector = resize(vector, size * elem_size);
            *array_inout = vector;
            vector->size = size;
        }

        vector->count = count + 1;
        return ECS_OFFSET(vector, ECS_SIZEOF(ecs_vector_t) + elem_size * count);
    }

    vector = _ecs_vector_new(elem_size, 2);
    *array_inout = vector;
    vector->count = 1;
    vector->size = 2;
    return ECS_OFFSET(vector, ECS_SIZEOF(ecs_vector_t));
}

void ecs_vector_remove_last(
    ecs_vector_t *vector)
{
    if (vector && vector->count) vector->count --;
}

bool _ecs_vector_pop(
    ecs_vector_t *vector,
    ecs_size_t elem_size,
    void *value)
{
    if (!vector) {
        return false;
    }

    ecs_assert(vector->elem_size == elem_size, ECS_INTERNAL_ERROR);

    int32_t count = vector->count;
    if (!count) {
        return false;
    }

    void *elem = ECS_OFFSET(vector,
        ECS_SIZEOF(ecs_vector_t) + (count - 1) * elem_size);

    if (value) {
        ecs_os_memcpy(value, elem, elem_size);
    }

    ecs_vector_remove_last(vector);

    return true;
}

int32_t _ecs_vector_remove(
    ecs_vector_t *vector,
    ecs_size_t elem_size,
    int32_t index)
{
    if (!vector) {
        return 0;
    }

    ecs_assert(vector->elem_size == elem_size, ECS_INTERNAL_ERROR);
    
    int32_t count = vector->count;
    void *buffer = ECS_OFFSET(vector, ECS_SIZEOF(ecs_vector_t));
    void *elem = ECS_OFFSET(buffer, index * elem_size);

    ecs_assert(index < count, ECS_INVALID_PARAMETER);

    count --;
    if (index != count) {
        void *last_elem = ECS_OFFSET(buffer, elem_size * count);
        ecs_os_memcpy(elem, last_elem, elem_size);
    }

    vector->count = count;

    return count;
}

void _ecs_vector_reclaim(
    ecs_vector_t **array_inout,
    ecs_size_t elem_size)
{
    ecs_vector_t *vector = *array_inout;

    ecs_assert(vector->elem_size == elem_size, ECS_INTERNAL_ERROR);
    
    int32_t size = vector->size;
    int32_t count = vector->count;

    if (count < size) {
        size = count;
        vector = resize(vector, size * elem_size);
        vector->size = size;
        *array_inout = vector;
    }
}

int32_t ecs_vector_count(
    const ecs_vector_t *vector)
{
    if (!vector) {
        return 0;
    }
    return vector->count;
}

int32_t ecs_vector_size(
    const ecs_vector_t *vector)
{
    if (!vector) {
        return 0;
    }
    return vector->size;
}

int32_t _ecs_vector_set_size(
    ecs_vector_t **array_inout,
    ecs_size_t elem_size,
    int32_t elem_count)
{
    ecs_vector_t *vector = *array_inout;

    if (!vector) {
        *array_inout = _ecs_vector_new(elem_size, elem_count);
        return elem_count;
    } else {
        ecs_assert(vector->elem_size == elem_size, ECS_INTERNAL_ERROR);

        int32_t result = vector->size;

        if (elem_count < vector->count) {
            elem_count = vector->count;
        }

        if (result < elem_count) {
            vector = resize(vector, elem_count * elem_size);
            vector->size = elem_count;
            *array_inout = vector;
            result = elem_count;
        }

        return result;
    }
}

int32_t _ecs_vector_grow(
    ecs_vector_t **array_inout,
    ecs_size_t elem_size,
    int32_t elem_count)
{
    int32_t current = ecs_vector_count(*array_inout);
    return _ecs_vector_set_size(array_inout, elem_size, current + elem_count);
}

int32_t _ecs_vector_set_count(
    ecs_vector_t **array_inout,
    ecs_size_t elem_size,
    int32_t elem_count)
{
    if (!*array_inout) {
        *array_inout = _ecs_vector_new(elem_size, elem_count);
    }

    ecs_assert((*array_inout)->elem_size == elem_size, ECS_INTERNAL_ERROR);

    (*array_inout)->count = elem_count;

    ecs_size_t size = _ecs_vector_set_size(
        array_inout, elem_size, ecs_next_pow_of_2(elem_count));
    return size;
}

void* _ecs_vector_first(
    const ecs_vector_t *vector,
    ecs_size_t elem_size)
{
    (void)elem_size;

    ecs_assert(!vector || vector->elem_size == elem_size, ECS_INTERNAL_ERROR);
    if (vector && vector->size) {
        return ECS_OFFSET(vector, ECS_SIZEOF(ecs_vector_t));
    } else {
        return NULL;
    }
}

void* _ecs_vector_get(
    const ecs_vector_t *vector,
    ecs_size_t elem_size,
    int32_t index)
{
    if (!vector) {
        return NULL;
    }
    
    ecs_assert(vector->elem_size == elem_size, ECS_INTERNAL_ERROR);    
    ecs_assert(index >= 0, ECS_INTERNAL_ERROR);

    int32_t count = vector->count;

    if (index >= count) {
        return NULL;
    }

    return ECS_OFFSET(vector, ECS_SIZEOF(ecs_vector_t) + elem_size * index);
}

void* _ecs_vector_last(
    const ecs_vector_t *vector,
    ecs_size_t elem_size)
{
    if (vector) {
        ecs_assert(vector->elem_size == elem_size, ECS_INTERNAL_ERROR);
        int32_t count = vector->count;
        if (!count) {
            return NULL;
        } else {
            return ECS_OFFSET(vector,   
                ECS_SIZEOF(ecs_vector_t) + elem_size * (count - 1));
        }
    } else {
        return NULL;
    }
}

int32_t _ecs_vector_set_min_size(
    ecs_vector_t **vector_inout,
    ecs_size_t elem_size,
    int32_t elem_count)
{
    if (!*vector_inout || (*vector_inout)->size < elem_count) {
        return _ecs_vector_set_size(vector_inout, elem_size, elem_count);
    } else {
        return (*vector_inout)->size;
    }
}

int32_t _ecs_vector_set_min_count(
    ecs_vector_t **vector_inout,
    ecs_size_t elem_size,
    int32_t elem_count)
{
    _ecs_vector_set_min_size(vector_inout, elem_size, elem_count);

    ecs_vector_t *v = *vector_inout;
    if (v && v->count < elem_count) {
        v->count = elem_count;
    }

    return v->count;
}

void _ecs_vector_sort(
    ecs_vector_t *vector,
    ecs_size_t elem_size,
    ecs_comparator_t compare_action)
{
    if (!vector) {
        return;
    }

    ecs_assert(vector->elem_size == elem_size, ECS_INTERNAL_ERROR);    

    int32_t count = vector->count;
    void *buffer = ECS_OFFSET(vector, ECS_SIZEOF(ecs_vector_t));

    if (count > 1) {
        qsort(buffer, (size_t)count, (size_t)elem_size, compare_action);
    }
}

void _ecs_vector_memory(
    const ecs_vector_t *vector,
    ecs_size_t elem_size,
    int32_t *allocd,
    int32_t *used)
{
    if (!vector) {
        return;
    }

    ecs_assert(vector->elem_size == elem_size, ECS_INTERNAL_ERROR);

    if (allocd) {
        *allocd += vector->size * elem_size + ECS_SIZEOF(ecs_vector_t);
    }
    if (used) {
        *used += vector->count * elem_size;
    }
}

ecs_vector_t* _ecs_vector_copy(
    const ecs_vector_t *src,
    ecs_size_t elem_size)
{
    if (!src) {
        return NULL;
    }

    ecs_vector_t *dst = _ecs_vector_new(elem_size, src->size);
    ecs_os_memcpy(dst, src, ECS_SIZEOF(ecs_vector_t) + elem_size * src->count);
    return dst;
}

#define PAGE_COUNT (4096)
#define PAGE_SIZE(paged) (paged->column_count * ECS_SIZEOF(void*))
#define PAGE(index) ((int32_t)index >> 12)
#define OFFSET(index) ((int32_t)index & 0xFFF)
#define DATA(array, size, offset) (ECS_OFFSET(array, size * offset))

static
ecs_page_t* page_new(
    ecs_paged_t *paged,
    int32_t page_index)
{
    int32_t count = ecs_vector_count(paged->pages);
    ecs_page_t *pages;
    ecs_size_t page_size = PAGE_SIZE(paged);

    if (count <= page_index) {
        _ecs_vector_set_count(&paged->pages, page_size, page_index + 1);
        pages = _ecs_vector_first(paged->pages, page_size);
        ecs_os_memset( ECS_OFFSET(pages, count * page_size), 0, 
            (1 + page_index - count) * page_size);
    } else {
        pages = _ecs_vector_first(paged->pages, page_size);
    }

    ecs_assert(pages != NULL, ECS_INTERNAL_ERROR);

    ecs_page_t *result = ECS_OFFSET(pages, page_index * page_size);

    /* Initialize the data arrays with zero's to guarantee that data is 
     * always initialized. When an entry is removed, data is reset back to
     * zero. Initialize now, as this can take advantage of calloc. */
    int32_t i;
    for (i = 0; i < paged->column_count; i ++) {
        ecs_assert(paged->column_sizes[i] != 0, ECS_INTERNAL_ERROR);
        result[i] = ecs_os_calloc(paged->column_sizes[i] * PAGE_COUNT);
        ecs_assert(result[i] != NULL, ECS_OUT_OF_MEMORY);
    }
    
    return result;
}

static
ecs_page_t* page_get(
    const ecs_paged_t *paged,
    int32_t page_index)
{
    ecs_assert(page_index >= 0, ECS_INTERNAL_ERROR);
    ecs_page_t *result = _ecs_vector_get(
        paged->pages, PAGE_SIZE(paged), page_index);
    if (result && result[0]) {
        return result;
    } else {
        return NULL;
    }
}

static
ecs_page_t* page_ensure(
    ecs_paged_t *paged,
    int32_t page_index)
{
    ecs_page_t *page = page_get(paged, page_index);
    if (page) {
        return page;
    }

    return page_new(paged, page_index);
}

static
void page_free(
    ecs_paged_t *paged,
    ecs_page_t *page)
{
    int32_t i;
    for (i = 0; i < paged->column_count; i ++) {
        ecs_os_free(page[i]);
    }
}

void ecs_paged_init(
    ecs_paged_t *paged,
    int32_t column_count,
    ecs_size_t *column_sizes)
{
    ecs_size_t array_size = column_count * ECS_SIZEOF(ecs_size_t);
    paged->column_sizes = ecs_os_malloc(array_size);
    ecs_os_memcpy(paged->column_sizes, column_sizes, array_size);
    paged->column_count = column_count;
}

ecs_paged_t* ecs_paged_new(
    int32_t column_count,
    ecs_size_t *column_sizes)
{
    ecs_paged_t *result = ecs_os_calloc(sizeof(ecs_paged_t));
    ecs_paged_init(result, column_count, column_sizes);
    return result;
}

void ecs_paged_clear(
    ecs_paged_t *paged)
{
    ecs_assert(paged != NULL, ECS_INVALID_PARAMETER);

    ecs_size_t page_size = PAGE_SIZE(paged);
    ecs_page_t *pages = _ecs_vector_first(paged->pages, page_size);
    int32_t i, count = ecs_vector_count(paged->pages);

    for (i = 0; i < count; i ++) {
        page_free(paged, ECS_OFFSET(pages, page_size * i));
    }

    ecs_vector_free(paged->pages);

    paged->pages = NULL;
}

void ecs_paged_deinit(
    ecs_paged_t *paged)
{
    ecs_assert(paged != NULL, ECS_INVALID_PARAMETER);
    ecs_paged_clear(paged);
    ecs_os_free(paged->column_sizes);
    paged->column_sizes  = NULL;
}

void ecs_paged_free(
    ecs_paged_t *paged)
{
    ecs_paged_deinit(paged);
    ecs_os_free(paged);
}

void* _ecs_paged_get(
    const ecs_paged_t *paged,
    ecs_size_t size,
    int32_t index,
    int32_t column)
{
    ecs_assert(paged != NULL, ECS_INVALID_PARAMETER);
    ecs_assert(column < paged->column_count, ECS_INVALID_PARAMETER);
    ecs_assert(size == paged->column_sizes[column], ECS_INVALID_PARAMETER);
    ecs_page_t *p = page_get(paged, PAGE(index));
    if (p) {
        return ECS_OFFSET(p[column], size * OFFSET(index));
    }
    return NULL;
}

void* _ecs_paged_ensure(
    ecs_paged_t *paged,
    ecs_size_t size,
    int32_t index,
    int32_t column)
{
    ecs_assert(paged != NULL, ECS_INVALID_PARAMETER);
    ecs_assert(column < paged->column_count, ECS_INVALID_PARAMETER);
    ecs_assert(size == paged->column_sizes[column], ECS_INVALID_PARAMETER);
    ecs_page_t *p = page_ensure(paged, PAGE(index));
    ecs_assert(p != NULL, ECS_INTERNAL_ERROR);
    return ECS_OFFSET(p[column], size * OFFSET(index));
}

int32_t ecs_paged_add(
    ecs_paged_t *paged)
{
    ecs_assert(paged != NULL, ECS_INVALID_PARAMETER);
    int32_t count = paged->count ++;
    page_ensure(paged, PAGE(count));
    return count;
}

void ecs_paged_remove(
    ecs_paged_t *paged,
    int32_t index)
{
    int32_t i, count = -- paged->count;
    if (count > 1) {
        ecs_page_t *page_dst = page_get(paged, PAGE(index));
        ecs_page_t *page_src = page_get(paged, PAGE(count));

        for (i = 0; i < paged->column_count; i ++) {
            ecs_size_t size = paged->column_sizes[i];
            void *dst = ECS_OFFSET(page_dst[i], size * OFFSET(index));
            void *src = ECS_OFFSET(page_src[i], size * OFFSET(count));
            ecs_os_memcpy(dst, src, size);
        }
    }
}

ecs_page_t* ecs_paged_get_page(
    const ecs_paged_t *paged,
    int32_t index)
{
    return page_get(paged, PAGE(index));
}

ecs_page_t* ecs_paged_ensure_page(
    ecs_paged_t *paged,
    int32_t index)
{
    return page_ensure(paged, PAGE(index));
}

void* _ecs_paged_page_get(
    const ecs_paged_t *paged,
    ecs_page_t *page, 
    ecs_size_t size,
    int32_t index,
    int32_t column)
{
    ecs_assert(paged != NULL, ECS_INVALID_PARAMETER);
    ecs_assert(page != NULL, ECS_INVALID_PARAMETER);
    ecs_assert(size == paged->column_sizes[column], ECS_INVALID_PARAMETER);
    return ECS_OFFSET(page[column], size * OFFSET(index));
}

int32_t ecs_paged_count(
    const ecs_paged_t *paged)
{
    return paged->count;
}

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

static
void ctor_init_zero(
    void *ptr,
    size_t size,
    int32_t count)
{
    ecs_os_memset(ptr, 0, ecs_from_size_t(size) * count);
}

static
void init_partition(ecs_world_t *world, ecs_partition_t *p) {    
    /* Initialize root table */
    p->tables = ecs_sparse_new(ecs_table_t);

    /* Initialize table map */
    p->table_map = ecs_map_new(ecs_table_t*, 8);

    /* Initialize one root table per stage */
    ecs_init_root_table(world, &p->root);
}

ecs_world_t *ecs_init(void) {
    /* Initialize OS API which is an abstraction over OS-specific functions */
    ecs_os_init();

    ecs_trace_1("bootstrap");
    ecs_log_push();

    /* We really need the heap */
    ecs_assert(ecs_os_has_heap(), ECS_MISSING_OS_API);

    /* These we can do without */
    if (!ecs_os_has_threading()) {
        ecs_trace_1("threading not available");
    }
    if (!ecs_os_has_time()) {
        ecs_trace_1("time management not available");
    }

    /* Genesis */
    ecs_world_t *world = ecs_os_calloc(sizeof(ecs_world_t));
    ecs_assert(world != NULL, ECS_OUT_OF_MEMORY);

    /* So we can tell whether the world is actually a world, or a thread ctx */
    world->magic = ECS_WORLD_MAGIC;
    
    /* Create entity index, which manages entity ids and links to tables */
    world->entity_index = ecs_sparse_new(ecs_record_t);
    ecs_sparse_set_id_source(world->entity_index, &world->stats.last_id);
    
    /* Init default partition */
    init_partition(world, &world->store);

    /* Initialize lookup datastructure for component storage policies */
    world->store_policies = ecs_map_new(ecs_store_policy_t, 0);

    /* Initalize datastructures for component lifecycle action lookup */
    world->lc_hi = ecs_map_new(ecs_lifecycle_t, 0);  
    ecs_ptree_init(&world->stores, ecs_store_t);

    /* Initialize main stage, which stores and merges deferred operations */
    ecs_stage_init(world, &world->stage);
    world->stage.world = world;

    /* Bootstrap builtin entities, components and systems */
    ecs_bootstrap(world);

    ecs_log_pop();

    return world;
}

static
void fini_tables(
    ecs_world_t *world,
    ecs_partition_t *p)
{
    int32_t i, count = ecs_sparse_count(p->tables);

    for (i = 0; i < count; i ++) {
        ecs_table_t *t = ecs_sparse_get_elem(p->tables, ecs_table_t, i);
        ecs_table_free(world, t);
    }

    ecs_table_free(world, &p->root);
}

static
void fini_partition(
    ecs_world_t *world,
    ecs_partition_t *p)
{
    fini_tables(world, p);
    ecs_sparse_free(p->tables);
    ecs_map_free(p->table_map);
}

static
void fini_storages(
    ecs_world_t *world)
{
    ecs_ptree_iter_t it = ecs_ptree_iter(&world->stores);
    ecs_store_t *store;
    while ((store = ecs_ptree_next(&it, ecs_store_t))) {
        ecs_dense_t *sparse = store->sparse;
        if (sparse) {
            ecs_dense_free(store->sparse);
        }
    }

    ecs_ptree_fini(&world->stores);
}

/* Cleanup component lifecycle callbacks & systems */
static
void fini_component_lifecycle(
    ecs_world_t *world)
{
    ecs_map_free(world->lc_hi);
}

/* Cleanup stages */
static
void fini_stages(
    ecs_world_t *world)
{
    ecs_stage_deinit(world, &world->stage);
}

/* The destroyer of worlds */
int ecs_fini(
    ecs_world_t *world)
{
    assert(world->magic == ECS_WORLD_MAGIC);
    assert(!world->in_progress);

    fini_stages(world);

    ecs_sparse_free(world->entity_index);
    fini_partition(world, &world->store);
    fini_storages(world);

    fini_component_lifecycle(world);

    /* In case the application tries to use the memory of the freed world, this
     * will trigger an assert */
    world->magic = 0;

    ecs_increase_timer_resolution(0);

    /* End of the world */
    ecs_os_free(world);

    ecs_os_fini(); 

    return 0;
}

#ifdef FLECS_THREADING

ecs_stage_t *ecs_get_stage(
    ecs_world_t **world_ptr)
{
    ecs_world_t *world = *world_ptr;

    ecs_assert(world->magic == ECS_WORLD_MAGIC ||
               world->magic == ECS_THREAD_MAGIC,
               ECS_INTERNAL_ERROR);

    if (world->magic == ECS_WORLD_MAGIC) {
        return &world->stage;
    } else if (world->magic == ECS_THREAD_MAGIC) {
        ecs_thread_t *thread = (ecs_thread_t*)world;
        *world_ptr = thread->world;
        return thread->stage;
    }

    /* Silence warnings */
    return NULL;
}

#endif

int32_t ecs_get_thread_index(
    ecs_world_t *world)
{
    if (world->magic == ECS_THREAD_MAGIC) {
        ecs_thread_t *thr = (ecs_thread_t*)world;
        return thr->stage->id;
    } else if (world->magic == ECS_WORLD_MAGIC) {
        return 0;
    } else {
        ecs_abort(ECS_INTERNAL_ERROR);
    }
}

static
void ecs_notify_tables(
    ecs_world_t *world,
    ecs_table_event_t *event)
{
    ecs_sparse_t *tables = world->store.tables;
    int32_t i, count = ecs_sparse_count(tables);

    for (i = 0; i < count; i ++) {
        ecs_table_t *table = ecs_sparse_get_elem(tables, ecs_table_t, i);
        ecs_table_notify(world, table, event);
    }
}

void ecs_set_lifecycle(
    ecs_world_t *world,
    ecs_entity_t component,
    ecs_lifecycle_t *lifecycle)
{
#ifndef NDEBUG
    const EcsType *component_ptr = ecs_get(world, component, EcsType);

    /* Cannot register lifecycle actions for things that aren't a component */
    ecs_assert(component_ptr != NULL, ECS_INVALID_PARAMETER);

    /* Cannot register lifecycle actions for components with size 0 */
    ecs_assert(component_ptr->size != 0, ECS_INVALID_PARAMETER);
#endif

    ecs_lifecycle_t *lc = ecs_get_or_create_lifecycle(world, component);
    ecs_assert(lc != NULL, ECS_INTERNAL_ERROR);

    if (lc->is_set) {
        ecs_assert(lc->ctor == lifecycle->ctor, 
            ECS_INCONSISTENT_COMPONENT_ACTION);
        ecs_assert(lc->dtor == lifecycle->dtor, 
            ECS_INCONSISTENT_COMPONENT_ACTION);
        ecs_assert(lc->copy == lifecycle->copy, 
            ECS_INCONSISTENT_COMPONENT_ACTION);
        ecs_assert(lc->move == lifecycle->move, 
            ECS_INCONSISTENT_COMPONENT_ACTION);
    } else {
        *lc = *lifecycle;
        lc->is_set = true;

        /* If no constructor is set, invoking any of the other lifecycle actions 
         * is not safe as they will potentially access uninitialized memory. For 
         * ease of use, if no constructor is specified, set a default one that 
         * initializes the component to 0. */
        if (!lifecycle->ctor && (lifecycle->dtor || lifecycle->copy || lifecycle->move)) {
            lc->ctor = ctor_init_zero;   
        }

        ecs_notify_tables(world, &(ecs_table_event_t) {
            .kind = EcsTableComponentInfo,
            .component = component
        });
    }
}

bool ecs_component_has_actions(
    ecs_world_t *world,
    ecs_entity_t component)
{
    ecs_lifecycle_t *lc = ecs_get_lifecycle(world, component);
    return (lc != NULL) && lc->is_set;
}

ecs_lifecycle_t * ecs_get_lifecycle(
    ecs_world_t *world,
    ecs_entity_t id)
{
    ecs_assert(id != 0, ECS_INTERNAL_ERROR);
    ecs_assert(!ecs_has_flag(id, ECS_PAIR), ECS_INTERNAL_ERROR);

    if (id < ECS_HI_COMPONENT_ID) {
        ecs_lifecycle_t *lc = &world->lc_lo[id];
        if (lc->is_set) {
            return lc;
        } else {
            return NULL;
        }
    } else {
        return ecs_map_get(world->lc_hi, ecs_lifecycle_t, id);
    }
}

ecs_lifecycle_t * ecs_get_or_create_lifecycle(
    ecs_world_t *world,
    ecs_entity_t component)
{    
    ecs_lifecycle_t *lc = ecs_get_lifecycle(world, component);
    if (!lc) {
        if (component < ECS_HI_COMPONENT_ID) {
            lc = &world->lc_lo[component];
        } else {
            ecs_lifecycle_t t_info = { 0 };
            ecs_map_set(world->lc_hi, component, &t_info);
            lc = ecs_map_get(world->lc_hi, ecs_lifecycle_t, component);
            ecs_assert(lc != NULL, ECS_INTERNAL_ERROR); 
        }
    }

    return lc;
}

void ecs_dim(
    ecs_world_t *world,
    int32_t entity_count)
{
    assert(world->magic == ECS_WORLD_MAGIC);
    ecs_eis_set_size(world, entity_count + ECS_HI_COMPONENT_ID);
}

ecs_entity_t ecs_set_scope(
    ecs_world_t *world,
    ecs_entity_t scope)
{
    ecs_stage_t *stage = ecs_get_stage(&world);

    ecs_entity_t e = ecs_pair(EcsScope, scope);

    ecs_entity_t cur = stage->scope;
    stage->scope = scope;

    if (scope) {
        stage->scope_table = ecs_table_traverse_add(
            world, &world->store.root, e);
    } else {
        stage->scope_table = &world->store.root;
    }

    return cur;
}

ecs_entity_t ecs_get_scope(
    const ecs_world_t *world)
{
    /* Cast is safe, ecs_get_stage doesn't modify the world */
    ecs_stage_t *stage = ecs_get_stage((ecs_world_t**)&world);
    return stage->scope;
}

const EcsType* ecs_get_type_from_id(
    ecs_world_t *world,
    ecs_entity_t id)
{
    const EcsType *result;

    /* If the id is flagged as a tag, it has no associated type */
    if (ecs_has_flag(id, ECS_TAG)) {
        return 0;

    /* If the id is a pair, a type could either come from the role or subject */
    } else if (ecs_has_flag(id, ECS_PAIR)) {
        ecs_entity_t role = ecs_get_role(id);
        result = ecs_get(world, role, EcsType);
        if (result) {
            return result;
        }

        id = ecs_get_subject(id);
        result = ecs_get(world, id, EcsType);

    /* If it's a regular id, check if it has an associated type */
    } else {
        result = ecs_get(world, id, EcsType);
    }

    return result;
}

ecs_entity_t ecs_get_type_id_from_id(
    ecs_world_t *world,
    ecs_entity_t id)
{
    /* If the id is flagged as a tag, it has no associated type */
    if (ecs_has_flag(id, ECS_TAG)) {
        return 0;

    /* If the id is a pair, a type could either come from the role or subject */
    } else if (ecs_has_flag(id, ECS_PAIR)) {
        ecs_entity_t role = ecs_get_role(id);
        if (ecs_has(world, role, EcsType)) {
            return role;
        }

        id = ecs_get_subject(id);
        if (ecs_has(world, role, EcsType)) {
            return id;
        }

    /* If it's a regular id, check if it has an associated type */        
    } else {
        if (ecs_has(world, id, EcsType)) {
            return id;   
        }
    }

    return 0;
}

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

#ifndef _MSC_VER
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#endif

/*
-------------------------------------------------------------------------------
lookup3.c, by Bob Jenkins, May 2006, Public Domain.
  http://burtleburtle.net/bob/c/lookup3.c
-------------------------------------------------------------------------------
*/

#ifdef _MSC_VER
//FIXME
#else
#include <sys/param.h>  /* attempt to define endianness */
#endif
#ifdef linux
# include <endian.h>    /* attempt to define endianness */
#endif

/*
 * My best guess at if you are big-endian or little-endian.  This may
 * need adjustment.
 */
#if (defined(__BYTE_ORDER) && defined(__LITTLE_ENDIAN) && \
     __BYTE_ORDER == __LITTLE_ENDIAN) || \
    (defined(i386) || defined(__i386__) || defined(__i486__) || \
     defined(__i586__) || defined(__i686__) || defined(vax) || defined(MIPSEL))
# define HASH_LITTLE_ENDIAN 1
#elif (defined(__BYTE_ORDER) && defined(__BIG_ENDIAN) && \
       __BYTE_ORDER == __BIG_ENDIAN) || \
      (defined(sparc) || defined(POWERPC) || defined(mc68000) || defined(sel))
# define HASH_LITTLE_ENDIAN 0
#else
# define HASH_LITTLE_ENDIAN 0
#endif

#define rot(x,k) (((x)<<(k)) | ((x)>>(32-(k))))

/*
-------------------------------------------------------------------------------
mix -- mix 3 32-bit values reversibly.

This is reversible, so any information in (a,b,c) before mix() is
still in (a,b,c) after mix().

If four pairs of (a,b,c) inputs are run through mix(), or through
mix() in reverse, there are at least 32 bits of the output that
are sometimes the same for one pair and different for another pair.
This was tested for:
* pairs that differed by one bit, by two bits, in any combination
  of top bits of (a,b,c), or in any combination of bottom bits of
  (a,b,c).
* "differ" is defined as +, -, ^, or ~^.  For + and -, I transformed
  the output delta to a Gray code (a^(a>>1)) so a string of 1's (as
  is commonly produced by subtraction) look like a single 1-bit
  difference.
* the base values were pseudorandom, all zero but one bit set, or 
  all zero plus a counter that starts at zero.

Some k values for my "a-=c; a^=rot(c,k); c+=b;" arrangement that
satisfy this are
    4  6  8 16 19  4
    9 15  3 18 27 15
   14  9  3  7 17  3
Well, "9 15 3 18 27 15" didn't quite get 32 bits diffing
for "differ" defined as + with a one-bit base and a two-bit delta.  I
used http://burtleburtle.net/bob/hash/avalanche.html to choose 
the operations, constants, and arrangements of the variables.

This does not achieve avalanche.  There are input bits of (a,b,c)
that fail to affect some output bits of (a,b,c), especially of a.  The
most thoroughly mixed value is c, but it doesn't really even achieve
avalanche in c.

This allows some parallelism.  Read-after-writes are good at doubling
the number of bits affected, so the goal of mixing pulls in the opposite
direction as the goal of parallelism.  I did what I could.  Rotates
seem to cost as much as shifts on every machine I could lay my hands
on, and rotates are much kinder to the top and bottom bits, so I used
rotates.
-------------------------------------------------------------------------------
*/
#define mix(a,b,c) \
{ \
  a -= c;  a ^= rot(c, 4);  c += b; \
  b -= a;  b ^= rot(a, 6);  a += c; \
  c -= b;  c ^= rot(b, 8);  b += a; \
  a -= c;  a ^= rot(c,16);  c += b; \
  b -= a;  b ^= rot(a,19);  a += c; \
  c -= b;  c ^= rot(b, 4);  b += a; \
}

/*
-------------------------------------------------------------------------------
final -- final mixing of 3 32-bit values (a,b,c) into c

Pairs of (a,b,c) values differing in only a few bits will usually
produce values of c that look totally different.  This was tested for
* pairs that differed by one bit, by two bits, in any combination
  of top bits of (a,b,c), or in any combination of bottom bits of
  (a,b,c).
* "differ" is defined as +, -, ^, or ~^.  For + and -, I transformed
  the output delta to a Gray code (a^(a>>1)) so a string of 1's (as
  is commonly produced by subtraction) look like a single 1-bit
  difference.
* the base values were pseudorandom, all zero but one bit set, or 
  all zero plus a counter that starts at zero.

These constants passed:
 14 11 25 16 4 14 24
 12 14 25 16 4 14 24
and these came close:
  4  8 15 26 3 22 24
 10  8 15 26 3 22 24
 11  8 15 26 3 22 24
-------------------------------------------------------------------------------
*/
#define final(a,b,c) \
{ \
  c ^= b; c -= rot(b,14); \
  a ^= c; a -= rot(c,11); \
  b ^= a; b -= rot(a,25); \
  c ^= b; c -= rot(b,16); \
  a ^= c; a -= rot(c,4);  \
  b ^= a; b -= rot(a,14); \
  c ^= b; c -= rot(b,24); \
}


/*
 * hashlittle2: return 2 32-bit hash values
 *
 * This is identical to hashlittle(), except it returns two 32-bit hash
 * values instead of just one.  This is good enough for hash table
 * lookup with 2^^64 buckets, or if you want a second hash if you're not
 * happy with the first, or if you want a probably-unique 64-bit ID for
 * the key.  *pc is better mixed than *pb, so use *pc first.  If you want
 * a 64-bit value do something like "*pc + (((uint64_t)*pb)<<32)".
 */
static
void hashlittle2( 
  const void *key,       /* the key to hash */
  size_t      length,    /* length of the key */
  uint32_t   *pc,        /* IN: primary initval, OUT: primary hash */
  uint32_t   *pb)        /* IN: secondary initval, OUT: secondary hash */
{
  uint32_t a,b,c;                                          /* internal state */
  union { const void *ptr; size_t i; } u;     /* needed for Mac Powerbook G4 */

  /* Set up the internal state */
  a = b = c = 0xdeadbeef + ((uint32_t)length) + *pc;
  c += *pb;

  u.ptr = key;
  if (HASH_LITTLE_ENDIAN && ((u.i & 0x3) == 0)) {
    const uint32_t *k = (const uint32_t *)key;         /* read 32-bit chunks */
    const uint8_t  *k8;
    (void)k8;

    /*------ all but last block: aligned reads and affect 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += k[0];
      b += k[1];
      c += k[2];
      mix(a,b,c);
      length -= 12;
      k += 3;
    }

    /*----------------------------- handle the last (probably partial) block */
    /* 
     * "k[2]&0xffffff" actually reads beyond the end of the string, but
     * then masks off the part it's not allowed to read.  Because the
     * string is aligned, the masked-off tail is in the same word as the
     * rest of the string.  Every machine with memory protection I've seen
     * does it on word boundaries, so is OK with this.  But VALGRIND will
     * still catch it and complain.  The masking trick does make the hash
     * noticably faster for short strings (like English words).
     */
#ifndef VALGRIND

    switch(length)
    {
    case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
    case 11: c+=k[2]&0xffffff; b+=k[1]; a+=k[0]; break;
    case 10: c+=k[2]&0xffff; b+=k[1]; a+=k[0]; break;
    case 9 : c+=k[2]&0xff; b+=k[1]; a+=k[0]; break;
    case 8 : b+=k[1]; a+=k[0]; break;
    case 7 : b+=k[1]&0xffffff; a+=k[0]; break;
    case 6 : b+=k[1]&0xffff; a+=k[0]; break;
    case 5 : b+=k[1]&0xff; a+=k[0]; break;
    case 4 : a+=k[0]; break;
    case 3 : a+=k[0]&0xffffff; break;
    case 2 : a+=k[0]&0xffff; break;
    case 1 : a+=k[0]&0xff; break;
    case 0 : *pc=c; *pb=b; return;  /* zero length strings require no mixing */
    }

#else /* make valgrind happy */

    k8 = (const uint8_t *)k;
    switch(length)
    {
    case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
    case 11: c+=((uint32_t)k8[10])<<16;  /* fall through */
    case 10: c+=((uint32_t)k8[9])<<8;    /* fall through */
    case 9 : c+=k8[8];                   /* fall through */
    case 8 : b+=k[1]; a+=k[0]; break;
    case 7 : b+=((uint32_t)k8[6])<<16;   /* fall through */
    case 6 : b+=((uint32_t)k8[5])<<8;    /* fall through */
    case 5 : b+=k8[4];                   /* fall through */
    case 4 : a+=k[0]; break;
    case 3 : a+=((uint32_t)k8[2])<<16;   /* fall through */
    case 2 : a+=((uint32_t)k8[1])<<8;    /* fall through */
    case 1 : a+=k8[0]; break;
    case 0 : *pc=c; *pb=b; return;  /* zero length strings require no mixing */
    }

#endif /* !valgrind */

  } else if (HASH_LITTLE_ENDIAN && ((u.i & 0x1) == 0)) {
    const uint16_t *k = (const uint16_t *)key;         /* read 16-bit chunks */
    const uint8_t  *k8;

    /*--------------- all but last block: aligned reads and different mixing */
    while (length > 12)
    {
      a += k[0] + (((uint32_t)k[1])<<16);
      b += k[2] + (((uint32_t)k[3])<<16);
      c += k[4] + (((uint32_t)k[5])<<16);
      mix(a,b,c);
      length -= 12;
      k += 6;
    }

    /*----------------------------- handle the last (probably partial) block */
    k8 = (const uint8_t *)k;
    switch(length)
    {
    case 12: c+=k[4]+(((uint32_t)k[5])<<16);
             b+=k[2]+(((uint32_t)k[3])<<16);
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 11: c+=((uint32_t)k8[10])<<16;     /* fall through */
    case 10: c+=k[4];
             b+=k[2]+(((uint32_t)k[3])<<16);
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 9 : c+=k8[8];                      /* fall through */
    case 8 : b+=k[2]+(((uint32_t)k[3])<<16);
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 7 : b+=((uint32_t)k8[6])<<16;      /* fall through */
    case 6 : b+=k[2];
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 5 : b+=k8[4];                      /* fall through */
    case 4 : a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 3 : a+=((uint32_t)k8[2])<<16;      /* fall through */
    case 2 : a+=k[0];
             break;
    case 1 : a+=k8[0];
             break;
    case 0 : *pc=c; *pb=b; return;  /* zero length strings require no mixing */
    }

  } else {                        /* need to read the key one byte at a time */
    const uint8_t *k = (const uint8_t *)key;

    /*--------------- all but the last block: affect some 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += k[0];
      a += ((uint32_t)k[1])<<8;
      a += ((uint32_t)k[2])<<16;
      a += ((uint32_t)k[3])<<24;
      b += k[4];
      b += ((uint32_t)k[5])<<8;
      b += ((uint32_t)k[6])<<16;
      b += ((uint32_t)k[7])<<24;
      c += k[8];
      c += ((uint32_t)k[9])<<8;
      c += ((uint32_t)k[10])<<16;
      c += ((uint32_t)k[11])<<24;
      mix(a,b,c);
      length -= 12;
      k += 12;
    }

    /*-------------------------------- last block: affect all 32 bits of (c) */
    switch(length)                   /* all the case statements fall through */
    {
    case 12: c+=((uint32_t)k[11])<<24;
    case 11: c+=((uint32_t)k[10])<<16;
    case 10: c+=((uint32_t)k[9])<<8;
    case 9 : c+=k[8];
    case 8 : b+=((uint32_t)k[7])<<24;
    case 7 : b+=((uint32_t)k[6])<<16;
    case 6 : b+=((uint32_t)k[5])<<8;
    case 5 : b+=k[4];
    case 4 : a+=((uint32_t)k[3])<<24;
    case 3 : a+=((uint32_t)k[2])<<16;
    case 2 : a+=((uint32_t)k[1])<<8;
    case 1 : a+=k[0];
             break;
    case 0 : *pc=c; *pb=b; return;  /* zero length strings require no mixing */
    }
  }

  final(a,b,c);
  *pc=c; *pb=b;
}

void ecs_hash(
    const void *data,
    ecs_size_t length,
    uint64_t *result)
{
    uint32_t h_1 = 0;
    uint32_t h_2 = 0;

    hashlittle2(
        data,
        ecs_to_size_t(length),
        &h_1,
        &h_2);

    *result = h_1 | ((uint64_t)h_2 << 32);
}

#define PTREE_PAGE_SIZE (65536)

static
addr_t to_addr(
    uint64_t id) 
{
    union {
        addr_t addr;
        uint64_t id;
    } u = {.id = id};
    return u.addr;
}

static
void* array_ensure(
    array_t *array,
    ecs_size_t elem_size,
    uint16_t index)
{
    int32_t offset = (int32_t)array->offset;
    int32_t length = array->length;

    ecs_assert(offset >= 0, ECS_INTERNAL_ERROR);
    ecs_assert(length >= 0, ECS_INTERNAL_ERROR);

    if (!length) {
        array->length = 1;
        array->offset = index;
        array->data = ecs_os_calloc(elem_size);
        ecs_assert(array->data != NULL, ECS_INTERNAL_ERROR);
        return array->data; 

    } else if (index < offset) {
        int32_t new_offset = ecs_next_pow_of_2(index) >> 1;
        int32_t dif_offset = offset - new_offset;
        int32_t new_length = ecs_next_pow_of_2(length + dif_offset);
        
        if ((new_offset + new_length) > PTREE_PAGE_SIZE) {
            new_offset = 0;
            new_length = PTREE_PAGE_SIZE;
            dif_offset = offset;
        }

        ecs_assert(length != new_length, ECS_INTERNAL_ERROR);
        array->data = ecs_os_realloc(array->data, new_length * elem_size);
        ecs_assert(array->data != NULL, ECS_INTERNAL_ERROR);

        ecs_os_memset(ECS_OFFSET(array->data, length * elem_size), 0, (new_length - length) * elem_size);
        ecs_os_memmove(ECS_OFFSET(array->data, dif_offset * elem_size), array->data, 
            length * elem_size);
        ecs_os_memset(array->data, 0, dif_offset * elem_size);            

        ecs_assert(new_offset >= 0, ECS_INTERNAL_ERROR);
        ecs_assert(new_offset < 65536, ECS_INTERNAL_ERROR);

        array->offset = (uint16_t)new_offset;
        array->length = new_length;

    } else if (index >= (offset + length)) {
        int32_t new_length = ecs_next_pow_of_2(index + offset + 1);
        if ((new_length + offset) > PTREE_PAGE_SIZE) {
            int32_t new_offset = ecs_next_pow_of_2(offset - ((new_length + offset) - PTREE_PAGE_SIZE)) >> 1;
            int32_t dif_offset = offset - new_offset;
            new_length = ecs_next_pow_of_2(new_offset + index);
            
            ecs_assert(length != new_length, ECS_INTERNAL_ERROR);
            array->data = ecs_os_realloc(array->data, new_length * elem_size);
            ecs_assert(array->data != NULL, ECS_INTERNAL_ERROR);

            ecs_os_memset(ECS_OFFSET(array->data, length * elem_size), 0, (new_length - length) * elem_size);
            ecs_os_memmove(ECS_OFFSET(array->data, dif_offset * elem_size), array->data, 
                length * elem_size);
            ecs_os_memset(array->data, 0, dif_offset * elem_size);

            ecs_assert(new_offset >= 0, ECS_INTERNAL_ERROR);
            ecs_assert(new_offset < 65536, ECS_INTERNAL_ERROR);

            array->offset = (uint16_t)new_offset;
        } else {
            ecs_assert(length != new_length, ECS_INTERNAL_ERROR);
            array->data = ecs_os_realloc(array->data, new_length * elem_size);
            ecs_assert(array->data != NULL, ECS_INTERNAL_ERROR);

            ecs_os_memset(ECS_OFFSET(array->data, length * elem_size), 0, 
                (new_length - length) * elem_size);
        }

        array->length = new_length;
    }

    ecs_assert((array->offset + array->length) <= PTREE_PAGE_SIZE, 
        ECS_INTERNAL_ERROR);
    
    return ECS_OFFSET(array->data, (index - array->offset) * elem_size);
}

static
void* array_get(
    const array_t *array,
    ecs_size_t elem_size,
    int32_t index)
{
    uint16_t offset = array->offset;
    int32_t length = array->length;

    if (index < offset) {
        return NULL;
    }

    index -= offset;
    if (index >= length) {
        return NULL;
    }

    return ECS_OFFSET(array->data, index * elem_size);
}

static
const page_t* get_page(
    const page_t *p,
    uint16_t *addr,
    int count) 
{
    int32_t i;
    for (i = count; i > 0; i --) {
        p = array_get(&p->pages, ECS_SIZEOF(page_t), addr[i]);
        if (!p) {
            return NULL;
        }
    }

    return p;
}

static
page_t* get_or_create_page(
    page_t *p,
    uint16_t *addr,
    int count)
{
    int32_t i;
    for (i = count; i > 0; i --) {
        p = array_ensure(&p->pages, ECS_SIZEOF(page_t), addr[i]);
        ecs_assert(p != NULL, ECS_INTERNAL_ERROR);
    }

    return p;
}

static
int8_t page_count(
    uint64_t index)
{
    return (int8_t)((index > 65535) +
        (index > 0x00000000FFFF0000) +
        (index > 0x0000FFFF00000000));
}

static
int32_t ptree_page_free(
    ecs_ptree_t *ptree,
    page_t *p)
{
    int32_t result = sizeof(page_t);

    if (p->data.data) {
        result += p->data.length * ECS_SIZEOF(ptree->elem_size);
        ecs_os_free(p->data.data);
    }

    if (p->pages.data) {
        int32_t i;
        for (i = 0; i < p->pages.length; i ++) {
            result += ptree_page_free(ptree, array_get(&p->pages, ECS_SIZEOF(page_t), 
                i + p->pages.offset));
        }

        ecs_os_free(p->pages.data);
    }

    return result;
}

static
uint64_t index_from_it(
    ecs_ptree_iter_t *it,
    page_t *page)
{
    ecs_assert(it->cur_elem >= 0, ECS_INTERNAL_ERROR);
    
    uint64_t result = page->data.offset + (uint32_t)it->cur_elem;
    int8_t sp = it->sp;

    int8_t i;
    for (i = 0; i <= sp; i ++) {
        result += (uint64_t)((it->cur_page[i] + 
            ((page_t*)it->frames[i])->pages.offset)) << (16 * (1 + sp - i));
    }

    return result;
}

void _ecs_ptree_init(
    ecs_ptree_t *ptree,
    ecs_size_t elem_size)
{
    ptree->first_65k = ecs_os_calloc(elem_size * 65536);
    ptree->elem_size = (uint8_t)elem_size;
}

void _ecs_ptiny_init(
    ecs_ptree_t *ptree,
    ecs_size_t elem_size)
{
    ecs_assert(ptree != NULL, ECS_OUT_OF_MEMORY);
    ecs_assert(elem_size < 256, ECS_INVALID_PARAMETER);
    ptree->elem_size = (uint8_t)elem_size;
    ptree->min_65k = 65535;
}

ecs_ptree_t* _ecs_ptiny_new(
    ecs_size_t elem_size)
{
    ecs_ptree_t *result = ecs_os_calloc(sizeof(ecs_ptree_t));
    _ecs_ptiny_init(result, elem_size);
    return result;
}

ecs_ptree_t* _ecs_ptree_new(
    ecs_size_t elem_size)
{
    ecs_ptree_t *result = _ecs_ptiny_new(elem_size);
    _ecs_ptree_init(result, elem_size);
    return result;
}

int32_t ecs_ptree_fini(
    ecs_ptree_t *ptree)
{
    int32_t result = ptree_page_free(ptree, &ptree->root);
    result += ECS_SIZEOF(int) * 65536;
    result += ECS_SIZEOF(ecs_ptree_t);
    ecs_os_free(ptree->first_65k);
    return result;
}

int32_t ecs_ptree_free(
    ecs_ptree_t *ptree)
{
    if (!ptree) {
        return 0;
    }

    int32_t result = ecs_ptree_fini(ptree);
    ecs_os_free(ptree);
    return result;
}

ecs_ptree_iter_t ecs_ptiny_iter(
    ecs_ptree_t *ptree)
{
    return (ecs_ptree_iter_t){
        .ptree = ptree,
        .frames[0] = &ptree->root,
        .index = (uint64_t)ptree->root.data.offset - 1
    };
}

ecs_ptree_iter_t ecs_ptree_iter(
    ecs_ptree_t *ptree)
{
    return (ecs_ptree_iter_t){
        .ptree = ptree,
        .frames[0] = &ptree->root,
        .index = (uint64_t)ptree->min_65k - 1
    };
}

void* _ecs_ptiny_next(
    ecs_ptree_iter_t *it,
    ecs_size_t elem_size)
{
    int8_t sp = it->sp;
    uint16_t cur_page = it->cur_page[sp];
    int32_t cur_elem = it->cur_elem;

    ecs_ptree_t *pt = it->ptree;
    if ((it->index == (uint64_t)-1) || (it->index <= UINT16_MAX)) {
        array_t *root_data = &pt->root.data;
        ecs_assert((root_data->offset + root_data->length) <= 65536, 
            ECS_INTERNAL_ERROR);
        uint32_t max = (uint32_t)(root_data->offset + root_data->length);
        uint64_t index = ++ it->index;
        if (index < max) {
            return array_get(&pt->root.data, elem_size, (uint16_t)index);
        }
    }

    do {
        page_t *frame = it->frames[sp];

        if (cur_page < frame->pages.length) {
            page_t *page = ECS_OFFSET(frame->pages.data, 
                cur_page * sizeof(page_t));

            if (cur_elem < page->data.length) {
                void *result = ECS_OFFSET(page->data.data, cur_elem * elem_size);
                ecs_assert(result != NULL, ECS_INTERNAL_ERROR);
                it->index = index_from_it(it, page);
                it->cur_elem ++;
                return result;
            }

            if (page->pages.length) {
                sp = ++ it->sp;
                ecs_assert(sp < 3, ECS_INTERNAL_ERROR);
                cur_page = it->cur_page[sp] = 0;
                it->frames[sp] = page;
                cur_elem = it->cur_elem = 0;
                continue;
            }

            if (cur_page < (frame->pages.length - 1)) {
                cur_page = ++ it->cur_page[sp];
                cur_elem = it->cur_elem = 0;
                continue;
            }
        }

        if (sp) {
            sp = -- it->sp;
            cur_page = ++ it->cur_page[sp];
            continue;
        }

        return NULL;
    } while (1);
}

void* _ecs_ptree_next(
    ecs_ptree_iter_t *it,
    ecs_size_t elem_size)
{
    ecs_ptree_t *pt = it->ptree;
    if ((it->index == (uint64_t)-1) || (it->index < pt->max_65k)) {
        uint64_t index = ++ it->index;
        return ECS_OFFSET(pt->first_65k, index * pt->elem_size);
    } else {
        return _ecs_ptiny_next(it, elem_size);
    }
}

void* _ecs_ptiny_ensure(
    ecs_ptree_t *ptree,
    ecs_size_t elem_size,
    uint64_t index)
{
    ecs_assert(elem_size == (ecs_size_t)ptree->elem_size, ECS_INTERNAL_ERROR);

    addr_t addr = to_addr(index);
    page_t *p = get_or_create_page(&ptree->root, addr.value, page_count(index));

    ecs_assert(p != NULL, ECS_INTERNAL_ERROR);
    void *data = array_ensure(&p->data, elem_size, addr.value[0]);
    ecs_assert(data != NULL, ECS_INTERNAL_ERROR);
    return data;
}

void* _ecs_ptree_ensure(
    ecs_ptree_t *ptree,
    ecs_size_t elem_size,
    uint64_t index)
{
    ecs_assert(elem_size == (ecs_size_t)ptree->elem_size, ECS_INTERNAL_ERROR);
    ecs_assert(ptree->first_65k != NULL, ECS_INTERNAL_ERROR);

    if (index < 65536) {
        if (index > ptree->max_65k) {
            ptree->max_65k = (uint16_t)index;
        }
        if (index < ptree->min_65k) {
            ptree->min_65k = (uint16_t)index;
        }
        return ECS_OFFSET(ptree->first_65k, (uint8_t)elem_size * index);
    } else {
       return _ecs_ptiny_ensure(ptree, (uint8_t)elem_size, index);
    }
}

void* _ecs_ptiny_get(
    const ecs_ptree_t *ptree,
    ecs_size_t elem_size,
    uint64_t index)
{
    ecs_assert(elem_size == (ecs_size_t)ptree->elem_size, ECS_INTERNAL_ERROR);

    if (index < 65536) {
        return array_get(&ptree->root.data, elem_size, (uint16_t)index);
    }

    int32_t pcount = page_count(index);
    addr_t addr = to_addr(index);
    const page_t *p = get_page(&ptree->root, addr.value, pcount);
    if (!p) {
        return NULL;
    }
    return array_get(&p->data, elem_size, addr.value[0]);
}

void* _ecs_ptree_get(
    const ecs_ptree_t *ptree,
    ecs_size_t elem_size,
    uint64_t index)
{
    ecs_assert(elem_size == (ecs_size_t)ptree->elem_size, ECS_INTERNAL_ERROR);
    ecs_assert(ptree->first_65k != NULL, ECS_INTERNAL_ERROR);

    if (index < 65536) {
        return ECS_OFFSET(ptree->first_65k, elem_size * (uint16_t)index);
    } else {
        return _ecs_ptiny_get(ptree, (uint8_t)elem_size, index);
    }
}


/* -- Private functions -- */

/* O(n) algorithm to check whether type 1 is equal or superset of type 2 */
ecs_entity_t ecs_type_contains(
    ecs_world_t *world,
    ecs_type_t type_1,
    ecs_type_t type_2,
    bool match_all)
{
    ecs_assert(world != NULL, ECS_INTERNAL_ERROR);
    ecs_unwrap_world(&world);
    
    if (!type_1) {
        return 0;
    }

    ecs_assert(type_2 != NULL, ECS_INTERNAL_ERROR);

    if (type_1 == type_2) {
        return *(ecs_vector_first(type_1, ecs_entity_t));
    }

    int32_t i_2, i_1 = 0;
    ecs_entity_t e1 = 0;
    ecs_entity_t *t1_array = ecs_vector_first(type_1, ecs_entity_t);
    ecs_entity_t *t2_array = ecs_vector_first(type_2, ecs_entity_t);

    ecs_assert(t1_array != NULL, ECS_INTERNAL_ERROR);
    ecs_assert(t2_array != NULL, ECS_INTERNAL_ERROR);

    int32_t t1_count = ecs_vector_count(type_1);
    int32_t t2_count = ecs_vector_count(type_2);

    for (i_2 = 0; i_2 < t2_count; i_2 ++) {
        ecs_entity_t e2 = t2_array[i_2];

        if (i_1 >= t1_count) {
            return 0;
        }

        e1 = t1_array[i_1];

        if (e2 > e1) {
            do {
                i_1 ++;
                if (i_1 >= t1_count) {
                    return 0;
                }
                e1 = t1_array[i_1];
            } while (e2 > e1);
        }

        if (e1 != e2) {
            if (e1 != e2) {
                if (match_all) return 0;
            } else if (!match_all) {
                return e1;
            }
        } else {
            if (!match_all) return e1;
            i_1 ++;
            if (i_1 < t1_count) {
                e1 = t1_array[i_1];
            }
        }
    }

    if (match_all) {
        return e1;
    } else {
        return 0;
    }
}

/* -- Public API -- */

int32_t ecs_type_index_of(
    ecs_type_t type,
    ecs_entity_t entity)
{
    ecs_vector_each(type, ecs_entity_t, c_ptr, {
        if (*c_ptr == entity) {
            return c_ptr_i; 
        }
    });

    return -1;
}

static
int match_entity(
    const ecs_world_t *world,
    ecs_type_t type,
    ecs_entity_t e,
    ecs_entity_t match_with)
{
    (void)world;
    ecs_entity_t role = ecs_get_role(match_with);

    if (role) {
        if (!ecs_has_role(role, e)) {
            return 0;
        }     

        ecs_entity_t id = match_with & ECS_ENTITY_MASK;
        ecs_entity_t comp = e & ECS_ENTITY_MASK;

        if (id == EcsWildcard) {
            ecs_entity_t *ids = ecs_vector_first(type, ecs_entity_t);
            int32_t i, count = ecs_vector_count(type);

            for (i = 0; i < count; i ++) {
                if (comp == ids[i]) {
                    return 2;
                }
            }

            return -1;
        }
    }

    if (e == match_with) {
        return 1;
    }

    return 0;
}

static
bool search_type(
    const ecs_world_t *world,
    ecs_type_t type,
    ecs_entity_t entity,
    bool owned)
{
    (void)owned;
    
    if (!type) {
        return false;
    }

    if (!entity) {
        return true;
    }

    ecs_entity_t *ids = ecs_vector_first(type, ecs_entity_t);
    int32_t i, count = ecs_vector_count(type);
    int matched = 0;

    for (i = 0; i < count; i ++) {
        int ret = match_entity(world, type, ids[i], entity);
        switch(ret) {
        case 0: break;
        case 1: return true;
        case -1: return false;
        case 2: matched ++; break;
        default: ecs_abort(ECS_INTERNAL_ERROR);
        }
    }

    return matched != 0;
}

bool ecs_type_has_entity(
    const ecs_world_t *world,
    ecs_type_t type,
    ecs_entity_t entity)
{
    return search_type(world, type, entity, false);
}

char* ecs_type_str(
    ecs_world_t *world,
    ecs_type_t type)
{
    if (!type) {
        return ecs_os_strdup("");
    }

    ecs_vector_t *chbuf = ecs_vector_new(char, 32);
    char *dst;

    ecs_entity_t *entities = ecs_vector_first(type, ecs_entity_t);
    int32_t i, count = ecs_vector_count(type);

    for (i = 0; i < count; i ++) {
        ecs_entity_t e = entities[i];
        char buffer[256];
        ecs_size_t len;

        if (i) {
            *(char*)ecs_vector_add(&chbuf, char) = ',';
        }

        if (e == 1) {
            ecs_os_strcpy(buffer, "EcsType");
            len = ecs_os_strlen("EcsType");
        } else {
            len = ecs_from_size_t(ecs_entity_str(world, e, buffer, 256));
        }

        dst = ecs_vector_addn(&chbuf, char, len);
        ecs_os_memcpy(dst, buffer, len);
    }

    *(char*)ecs_vector_add(&chbuf, char) = '\0';

    char* result = ecs_os_strdup(ecs_vector_first(chbuf, char));
    ecs_vector_free(chbuf);
    return result;
}

ecs_entities_t ecs_type_to_entities(
    ecs_type_t type)
{
    return (ecs_entities_t){
        .array = ecs_vector_first(type, ecs_entity_t),
        .count = ecs_vector_count(type)
    };
}

void ecs_os_api_impl(ecs_os_api_t *api);

static bool ecs_os_api_initialized = false;
static int ecs_os_api_init_count = 0;

ecs_os_api_t ecs_os_api;

int64_t ecs_os_api_malloc_count = 0;
int64_t ecs_os_api_realloc_count = 0;
int64_t ecs_os_api_calloc_count = 0;
int64_t ecs_os_api_free_count = 0;

void ecs_os_set_api(
    ecs_os_api_t *os_api)
{
    if (!ecs_os_api_initialized) {
        ecs_os_api = *os_api;
        ecs_os_api_initialized = true;
    }
}

void ecs_os_init(void)
{
    if (!ecs_os_api_initialized) {
        ecs_os_set_api_defaults();
    }
    
    if (!(ecs_os_api_init_count ++)) {
        if (ecs_os_api.init_) {
            ecs_os_api.init_();
        }
    }
}

void ecs_os_fini(void) {
    if (!--ecs_os_api_init_count) {
        if (ecs_os_api.fini_) {
            ecs_os_api.fini_();
        }
    }
}

static
void ecs_log(const char *fmt, va_list args) {
    vfprintf(stdout, fmt, args);
    fprintf(stdout, "\n");
}

static
void ecs_log_error(const char *fmt, va_list args) {
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
}

static
void ecs_log_debug(const char *fmt, va_list args) {
    vfprintf(stdout, fmt, args);
    fprintf(stdout, "\n");
}

static
void ecs_log_warning(const char *fmt, va_list args) {
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
}

void ecs_os_dbg(const char *fmt, ...) {
#ifndef NDEBUG
    va_list args;
    va_start(args, fmt);
    if (ecs_os_api.log_debug_) {
        ecs_os_api.log_debug_(fmt, args);
    }
    va_end(args);
#else
    (void)fmt;
#endif
}

void ecs_os_warn(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    if (ecs_os_api.log_warning_) {
        ecs_os_api.log_warning_(fmt, args);
    }
    va_end(args);
}

void ecs_os_log(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    if (ecs_os_api.log_) {
        ecs_os_api.log_(fmt, args);
    }
    va_end(args);
}

void ecs_os_err(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    if (ecs_os_api.log_error_) {
        ecs_os_api.log_error_(fmt, args);
    }
    va_end(args);
}

void ecs_os_gettime(ecs_time_t *time) {
    uint64_t now = ecs_os_time_now();
    uint64_t sec = now / 1000000000;

    assert(sec < UINT32_MAX);
    assert((now - sec * 1000000000) < UINT32_MAX);

    time->sec = (uint32_t)sec;
    time->nanosec = (uint32_t)(now - sec * 1000000000);
}

static
void* ecs_os_api_malloc(ecs_size_t size) {
    ecs_os_api_malloc_count ++;
    ecs_assert(size > 0, ECS_INVALID_PARAMETER);
    return malloc((size_t)size);
}

static
void* ecs_os_api_calloc(ecs_size_t size) {
    ecs_os_api_calloc_count ++;
    ecs_assert(size > 0, ECS_INVALID_PARAMETER);
    return calloc(1, (size_t)size);
}

static
void* ecs_os_api_realloc(void *ptr, ecs_size_t size) {
    ecs_assert(size > 0, ECS_INVALID_PARAMETER);

    if (ptr) {
        ecs_os_api_realloc_count ++;
    } else {
        /* If not actually reallocing, treat as malloc */
        ecs_os_api_malloc_count ++; 
    }
    
    return realloc(ptr, (size_t)size);
}

static
void ecs_os_api_free(void *ptr) {
    if (ptr) {
        ecs_os_api_free_count ++;
    }
    free(ptr);
}

static
char* ecs_os_api_strdup(const char *str) {
    int len = ecs_os_strlen(str);
    char *result = ecs_os_malloc(len + 1);
    ecs_assert(result != NULL, ECS_OUT_OF_MEMORY);
    ecs_os_strcpy(result, str);
    return result;
}

static
char* ecs_os_api_module_to_dl(const char *module) {
    (void)module;
    return NULL;
}

static
char* ecs_os_api_module_to_etc(const char *module) {
    (void)module;
    return NULL;
}

void ecs_os_set_api_defaults(void)
{
    /* Don't overwrite if already initialized */
    if (ecs_os_api_initialized != 0) {
        return;
    }

    ecs_os_time_setup();
    
    /* Memory management */
    ecs_os_api.malloc_ = ecs_os_api_malloc;
    ecs_os_api.free_ = ecs_os_api_free;
    ecs_os_api.realloc_ = ecs_os_api_realloc;
    ecs_os_api.calloc_ = ecs_os_api_calloc;

    /* Strings */
    ecs_os_api.strdup_ = ecs_os_api_strdup;

    /* Time */
    ecs_os_api.sleep_ = ecs_os_time_sleep;
    ecs_os_api.get_time_ = ecs_os_gettime;

    /* Logging */
    ecs_os_api.log_ = ecs_log;
    ecs_os_api.log_error_ = ecs_log_error;
    ecs_os_api.log_debug_ = ecs_log_debug;
    ecs_os_api.log_warning_ = ecs_log_warning;

    /* Modules */
    if (!ecs_os_api.module_to_dl_) {
        ecs_os_api.module_to_dl_ = ecs_os_api_module_to_dl;
    }

    if (!ecs_os_api.module_to_etc_) {
        ecs_os_api.module_to_etc_ = ecs_os_api_module_to_etc;
    }

    ecs_os_api.abort_ = abort;
}

bool ecs_os_has_heap(void) {
    return 
        (ecs_os_api.malloc_ != NULL) &&
        (ecs_os_api.calloc_ != NULL) &&
        (ecs_os_api.realloc_ != NULL) &&
        (ecs_os_api.free_ != NULL);
}

bool ecs_os_has_threading(void) {
    return
        (ecs_os_api.mutex_new_ != NULL) &&
        (ecs_os_api.mutex_free_ != NULL) &&
        (ecs_os_api.mutex_lock_ != NULL) &&
        (ecs_os_api.mutex_unlock_ != NULL) &&
        (ecs_os_api.cond_new_ != NULL) &&
        (ecs_os_api.cond_free_ != NULL) &&
        (ecs_os_api.cond_wait_ != NULL) &&
        (ecs_os_api.cond_signal_ != NULL) &&
        (ecs_os_api.cond_broadcast_ != NULL) &&
        (ecs_os_api.thread_new_ != NULL) &&
        (ecs_os_api.thread_join_ != NULL);   
}

bool ecs_os_has_time(void) {
    return 
        (ecs_os_api.get_time_ != NULL) &&
        (ecs_os_api.sleep_ != NULL);
}

bool ecs_os_has_logging(void) {
    return 
        (ecs_os_api.log_ != NULL) &&
        (ecs_os_api.log_error_ != NULL) &&
        (ecs_os_api.log_debug_ != NULL) &&
        (ecs_os_api.log_warning_ != NULL);
}

bool ecs_os_has_dl(void) {
    return 
        (ecs_os_api.dlopen_ != NULL) &&
        (ecs_os_api.dlproc_ != NULL) &&
        (ecs_os_api.dlclose_ != NULL);  
}

bool ecs_os_has_modules(void) {
    return 
        (ecs_os_api.module_to_dl_ != NULL) &&
        (ecs_os_api.module_to_etc_ != NULL);
}

/* Count number of columns with data (excluding tags) */
static
int32_t data_column_count(
    ecs_world_t * world,
    ecs_table_t * table)
{
    int32_t count = 0;
    ecs_vector_each(table->type, ecs_entity_t, c_ptr, {
        ecs_entity_t component = *c_ptr;

        /* Typically all components will be clustered together at the start of
         * the type as components are created from a separate id pool, and type
         * vectors are sorted. 
         * Explicitly check for EcsType and EcsName since the ecs_has check
         * doesn't work during bootstrap. */
        const EcsType *c = NULL;
        if ((component == ecs_typeid(EcsType)) || 
            (component == ecs_typeid(EcsName)) || 
            (c = ecs_get_type_from_id(world, component)) != NULL) 
        {
            if (!c || c->size) {
                count = c_ptr_i + 1;
            }
        }
    });

    return count;
}

static
ecs_type_t entities_to_type(
    ecs_entities_t *entities)
{
    if (entities->count) {
        ecs_vector_t *result = NULL;
        ecs_vector_set_count(&result, ecs_entity_t, entities->count);
        ecs_entity_t *array = ecs_vector_first(result, ecs_entity_t);
        ecs_os_memcpy(array, entities->array, ECS_SIZEOF(ecs_entity_t) * entities->count);
        return result;
    } else {
        return NULL;
    }
}

static
ecs_edge_t* get_edge(
    ecs_table_t *node,
    ecs_entity_t e)
{
    if (e < ECS_HI_COMPONENT_ID) {
        if (!node->lo_edges) {
            node->lo_edges = ecs_os_calloc(sizeof(ecs_edge_t) * ECS_HI_COMPONENT_ID);
        }
        return &node->lo_edges[e];
    } else {
        if (!node->hi_edges) {
            node->hi_edges = ecs_map_new(ecs_edge_t, 1);
        }
        return ecs_map_ensure(node->hi_edges, ecs_edge_t, e);
    }
}

static
void init_edges(
    ecs_world_t * world,
    ecs_table_t * table)
{
    (void)world;

    ecs_entity_t *entities = ecs_vector_first(table->type, ecs_entity_t);
    int32_t count = ecs_vector_count(table->type);

    table->lo_edges = NULL;
    table->hi_edges = NULL;
    
    int32_t i;
    for (i = 0; i < count; i ++) {
        ecs_entity_t e = entities[i];

        ecs_edge_t *edge = get_edge(table, e);
        ecs_assert(edge != NULL, ECS_INTERNAL_ERROR);
        edge->add = table;

        if (count == 1) {
            edge->remove = &world->store.root;
        }

        /* As we're iterating over the table components, also set the table
         * flags. These allow us to quickly determine if the table contains
         * data that needs to be handled in a special way, like prefabs or 
         * containers */
        if (e <= EcsLastInternalComponentId) {
            table->flags |= EcsTableHasBuiltins;
        }

        if (e == EcsDisabled) {
            table->flags |= EcsTableIsDisabled;
        }

        if (e == ecs_typeid(EcsType)) {
            table->flags |= EcsTableHasComponentData;
        }
    }
}

static
void init_table(
    ecs_world_t * world,
    ecs_table_t * table,
    ecs_entities_t * entities)
{
    (void)world;

    table->type = entities_to_type(entities);
    table->column_count = data_column_count(world, table);

    uint64_t hash = 0;
    ecs_hash(entities->array, entities->count * ECS_SIZEOF(ecs_entity_t), &hash);
    ecs_map_set(world->store.table_map, hash, &table);

    init_edges(world, table);
}

static
ecs_table_t *create_table(
    ecs_world_t * world,
    ecs_entities_t * entities)
{
    ecs_table_t *result = ecs_sparse_add(world->store.tables, ecs_table_t);
    result->id = ecs_to_u32(ecs_sparse_last_id(world->store.tables));

    ecs_assert(result != NULL, ECS_INTERNAL_ERROR);
    init_table(world, result, entities);

    return result;
}

static
void remove_entity_from_type(
    ecs_type_t type,
    ecs_entity_t remove,
    ecs_entities_t *out)
{
    int32_t count = ecs_vector_count(type);
    ecs_entity_t *array = ecs_vector_first(type, ecs_entity_t);

    int32_t i, el = 0;
    for (i = 0; i < count; i ++) {
        ecs_entity_t e = array[i];
        if (e != remove) {
            out->array[el ++] = e;
            ecs_assert(el <= count, ECS_INTERNAL_ERROR);
        }
    }

    out->count = el;
}

static
void add_entity_to_type(
    ecs_type_t type,
    ecs_entity_t add,
    ecs_entities_t *out)
{
    int32_t count = ecs_vector_count(type);
    ecs_entity_t *array = ecs_vector_first(type, ecs_entity_t);    
    bool added = false;

    int32_t i, el = 0;
    for (i = 0; i < count; i ++) {
        ecs_entity_t e = array[i];

        if (e > add && !added) {
            out->array[el ++] = add;
            added = true;
        }
        
        out->array[el ++] = e;
        ecs_assert(el <= out->count, ECS_INTERNAL_ERROR);
    }

    if (!added) {
        out->array[el ++] = add;
    }

    out->count = el;
    ecs_assert(out->count != 0, ECS_INTERNAL_ERROR);
}

static
ecs_table_t *find_or_create_remove(
    ecs_world_t * world,
    ecs_table_t * node,
    ecs_entity_t remove)
{
    ecs_type_t type = node->type;
    int32_t count = ecs_vector_count(type);

    ecs_entities_t entities = {
        .array = ecs_os_alloca(ECS_SIZEOF(ecs_entity_t) * count),
        .count = count
    };

    remove_entity_from_type(type, remove, &entities);

    ecs_table_t *result = ecs_table_find_or_create(world, &entities);
    if (!result) {
        return NULL;
    }

    if (result != node) {
        ecs_edge_t *edge = get_edge(result, remove);
        if (!edge->add) {
            edge->add = node;
        }
    }

    return result;    
}

static
ecs_table_t *find_or_create_add(
    ecs_world_t * world,
    ecs_table_t * node,
    ecs_entity_t add)
{
    ecs_type_t type = node->type;
    int32_t count = ecs_vector_count(type);

    ecs_entities_t entities = {
        .array = ecs_os_alloca(ECS_SIZEOF(ecs_entity_t) * (count + 1)),
        .count = count + 1
    };

    add_entity_to_type(type, add, &entities);

    ecs_table_t *result = ecs_table_find_or_create(world, &entities);
    
    if (result != node) {
        ecs_edge_t *edge = get_edge(result, add);
        if (!edge->remove) {
            edge->remove = node;
        }
    }

    return result;
}

ecs_table_t* ecs_table_traverse_remove(
    ecs_world_t * world,
    ecs_table_t * node,
    ecs_entity_t e)    
{
    node = node ? node : &world->store.root;

    ecs_edge_t *edge = get_edge(node, e);
    ecs_table_t *next = edge->remove;

    if (!next) {
        next = find_or_create_remove(world, node, e);
        ecs_assert(next != NULL, ECS_INTERNAL_ERROR);
        edge->add = next;
    }

    return next;
}

ecs_table_t* ecs_table_traverse_add(
    ecs_world_t * world,
    ecs_table_t * node,
    ecs_entity_t e)    
{
    node = node ? node : &world->store.root;

    ecs_edge_t *edge = get_edge(node, e);
    ecs_table_t *next = edge->add;

    if (!next) {
        next = find_or_create_add(world, node, e);
        ecs_assert(next != NULL, ECS_INTERNAL_ERROR);
        edge->add = next;
    }

    return next;
}

static
int ecs_entity_compare(
    const void *e1,
    const void *e2)
{
    ecs_entity_t v1 = *(ecs_entity_t*)e1;
    ecs_entity_t v2 = *(ecs_entity_t*)e2;
    if (v1 < v2) {
        return -1;
    } else if (v1 > v2) {
        return 1;
    } else {
        return 0;
    }
}

static
bool ecs_entity_array_is_ordered(
    ecs_entities_t *entities)
{
    ecs_entity_t prev = 0;
    ecs_entity_t *array = entities->array;
    int32_t i, count = entities->count;

    for (i = 0; i < count; i ++) {
        if (!array[i] && !prev) {
            continue;
        }
        if (array[i] <= prev) {
            return false;
        }
        prev = array[i];
    }    

    return true;
}

static
int32_t ecs_entity_array_dedup(
    ecs_entity_t *array,
    int32_t count)
{
    int32_t j, k;
    ecs_entity_t prev = array[0];

    for (k = j = 1; k < count; j ++, k++) {
        ecs_entity_t e = array[k];
        if (e == prev) {
            k ++;
        }

        array[j] = e;
        prev = e;
    }

    return count - (k - j);
}

static
ecs_table_t *find_or_create(
    ecs_world_t * world,
    ecs_entities_t * entities)
{    
    ecs_assert(world != NULL, ECS_INTERNAL_ERROR);

    /* Make sure array is ordered and does not contain duplicates */
    int32_t type_count = entities->count;
    ecs_entity_t *ordered = NULL;

    if (!type_count) {
        return &world->store.root;
    }

    if (!ecs_entity_array_is_ordered(entities)) {
        ecs_size_t size = ECS_SIZEOF(ecs_entity_t) * type_count;
        ordered = ecs_os_alloca(size);
        ecs_os_memcpy(ordered, entities->array, size);
        qsort(ordered, (size_t)type_count, sizeof(ecs_entity_t), ecs_entity_compare);
        type_count = ecs_entity_array_dedup(ordered, type_count);
    } else {
        ordered = entities->array;
    }

    uint64_t hash = 0;
    ecs_hash(entities->array, entities->count * ECS_SIZEOF(ecs_entity_t), &hash);
    ecs_table_t *table = ecs_map_get_ptr(world->store.table_map, ecs_table_t*, hash);
    if (table) {
        return table;
    }

    ecs_entities_t ordered_entities = {
        .array = ordered,
        .count = type_count
    };

    /* If we get here, the table has not been found. It has to be created. */
    
    ecs_table_t *result = create_table(world, &ordered_entities);

    ecs_assert(ordered_entities.count == ecs_vector_count(result->type), 
        ECS_INTERNAL_ERROR);

    return result;
}

ecs_table_t* ecs_table_find_or_create(
    ecs_world_t * world,
    ecs_entities_t * components)
{
    ecs_assert(world != NULL, ECS_INTERNAL_ERROR);
    ecs_assert(!world->in_progress, ECS_INTERNAL_ERROR);

    return find_or_create(world, components);
}

ecs_table_t* ecs_table_from_type(
    ecs_world_t *world,
    ecs_type_t type)
{
    ecs_entities_t components = ecs_type_to_entities(type);
    return ecs_table_find_or_create(
        world, &components);
}

void ecs_init_root_table(
    ecs_world_t *world,
    ecs_table_t *root)
{
    ecs_entities_t entities = {
        .array = NULL,
        .count = 0
    };

    init_table(world, root, &entities);
}

void ecs_table_clear_edges(
    ecs_table_t *table)
{
    uint32_t i;

    if (table->lo_edges) {
        for (i = 0; i < ECS_HI_COMPONENT_ID; i ++) {
            ecs_edge_t *e = &table->lo_edges[i];
            ecs_table_t *add = e->add, *remove = e->remove;
            if (add) {
                add->lo_edges[i].remove = NULL;
            }
            if (remove) {
                remove->lo_edges[i].add = NULL;
            }
        }
    }

    ecs_map_iter_t it = ecs_map_iter(table->hi_edges);
    ecs_edge_t *edge;
    ecs_map_key_t component;
    while ((edge = ecs_map_next(&it, ecs_edge_t, &component))) {
        ecs_table_t *add = edge->add, *remove = edge->remove;
        if (add) {
            ecs_edge_t *e = get_edge(add, component);
            e->remove = NULL;
            if (!e->add) {
                ecs_map_remove(add->hi_edges, component);
            }
        }
        if (remove) {
            ecs_edge_t *e = get_edge(remove, component);
            e->add = NULL;
            if (!e->remove) {
                ecs_map_remove(remove->hi_edges, component);
            }
        }
    }
}

/* The ratio used to determine whether the map should rehash. If
 * (element_count * LOAD_FACTOR) > bucket_count, bucket count is increased. */
#define LOAD_FACTOR (1.5)
#define KEY_SIZE (ECS_SIZEOF(ecs_map_key_t))
#define GET_ELEM(array, elem_size, index) \
    ECS_OFFSET(array, (elem_size) * (index))

struct ecs_bucket_t {
    ecs_map_key_t *keys;    /* Array with keys */
    void *payload;          /* Payload array */
    int32_t count;          /* Number of elements in bucket */
};

struct ecs_map_t {
    ecs_bucket_t *buckets;
    int32_t elem_size;
    int32_t bucket_count;
    int32_t count;
};

/* Get bucket count for number of elements */
static
int32_t get_bucket_count(
    int32_t element_count)
{
    return ecs_next_pow_of_2((int32_t)((float)element_count * LOAD_FACTOR));
}

/* Get bucket index for provided map key */
static
int32_t get_bucket_id(
    int32_t bucket_count,
    ecs_map_key_t key) 
{
    ecs_assert(bucket_count > 0, ECS_INTERNAL_ERROR);
    int32_t result = (int32_t)(key & ((uint64_t)bucket_count - 1));
    ecs_assert(result < INT32_MAX, ECS_INTERNAL_ERROR);
    return result;
}

/* Get bucket for key */
static
ecs_bucket_t* get_bucket(
    const ecs_map_t *map,
    ecs_map_key_t key)
{
    int32_t bucket_count = map->bucket_count;
    if (!bucket_count) {
        return NULL;
    }

    int32_t bucket_id = get_bucket_id(bucket_count, key);
    ecs_assert(bucket_id < bucket_count, ECS_INTERNAL_ERROR);

    return &map->buckets[bucket_id];
}

/* Ensure that map has at least new_count buckets */
static
void ensure_buckets(
    ecs_map_t *map,
    int32_t new_count)
{
    int32_t bucket_count = map->bucket_count;
    new_count = ecs_next_pow_of_2(new_count);
    if (new_count && new_count > bucket_count) {
        map->buckets = ecs_os_realloc(map->buckets, new_count * ECS_SIZEOF(ecs_bucket_t));
        map->bucket_count = new_count;

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
    if (!map->bucket_count) {
        ensure_buckets(map, 2);
    }

    int32_t bucket_id = get_bucket_id(map->bucket_count, key);
    ecs_assert(bucket_id >= 0, ECS_INTERNAL_ERROR);
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
    bucket->payload = ecs_os_realloc(bucket->payload, elem_size * bucket_count);
    bucket->keys[index] = key;

    if (payload) {
        void *elem = GET_ELEM(bucket->payload, elem_size, index);
        ecs_os_memcpy(elem, payload, elem_size);
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

    ecs_assert(bucket->count != 0, ECS_INTERNAL_ERROR);
    ecs_assert(index < bucket->count, ECS_INTERNAL_ERROR);
    
    int32_t bucket_count = -- bucket->count;

    if (index != bucket->count) {
        ecs_assert(key == bucket->keys[index], ECS_INTERNAL_ERROR);
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
    ecs_assert(bucket_count != 0, ECS_INTERNAL_ERROR);
    ecs_assert(bucket_count > map->bucket_count, ECS_INTERNAL_ERROR);

    ecs_size_t elem_size = map->elem_size;

    ensure_buckets(map, bucket_count);

    ecs_bucket_t *buckets = map->buckets;
    ecs_assert(buckets != NULL, ECS_INTERNAL_ERROR);
    
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
            int32_t new_bucket_id = get_bucket_id(bucket_count, key);

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

ecs_map_t* _ecs_map_new(
    ecs_size_t elem_size,
    ecs_size_t alignment, 
    int32_t element_count)
{
    (void)alignment;

    ecs_map_t *result = ecs_os_calloc(ECS_SIZEOF(ecs_map_t) * 1);
    ecs_assert(result != NULL, ECS_OUT_OF_MEMORY);

    int32_t bucket_count = get_bucket_count(element_count);

    result->count = 0;
    result->elem_size = elem_size;

    ensure_buckets(result, bucket_count);

    return result;
}

void ecs_map_free(
    ecs_map_t *map)
{
    if (map) {
        clear_buckets(map);
        ecs_os_free(map);
    }
}

void* _ecs_map_get(
    const ecs_map_t *map,
    ecs_size_t elem_size,
    ecs_map_key_t key)
{
    (void)elem_size;

    if (!map) {
        return NULL;
    }

    ecs_assert(elem_size == map->elem_size, ECS_INVALID_PARAMETER);

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
    void * ptr_ptr = _ecs_map_get(map, ECS_SIZEOF(void*), key);

    if (ptr_ptr) {
        return *(void**)ptr_ptr;
    } else {
        return NULL;
    }
}

void * _ecs_map_ensure(
    ecs_map_t *map,
    ecs_size_t elem_size,
    ecs_map_key_t key)
{
    void *result = _ecs_map_get(map, elem_size, key);
    if (!result) {
        result = _ecs_map_set(map, elem_size, key, NULL);
        ecs_assert(result != NULL, ECS_INTERNAL_ERROR);
        ecs_os_memset(result, 0, elem_size);
    }

    return result;
}

void* _ecs_map_set(
    ecs_map_t *map,
    ecs_size_t elem_size,
    ecs_map_key_t key,
    const void *payload)
{
    ecs_assert(map != NULL, ECS_INVALID_PARAMETER);
    ecs_assert(elem_size == map->elem_size, ECS_INVALID_PARAMETER);

    ecs_bucket_t *bucket = ensure_bucket(map, key);
    ecs_assert(bucket != NULL, ECS_INTERNAL_ERROR);

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

void ecs_map_remove(
    ecs_map_t *map,
    ecs_map_key_t key)
{
    ecs_assert(map != NULL, ECS_INVALID_PARAMETER);

    ecs_bucket_t * bucket = get_bucket(map, key);
    if (!bucket) {
        return;
    }

    int32_t i, bucket_count = bucket->count;
    for (i = 0; i < bucket_count; i ++) {
        if (bucket->keys[i] == key) {
            remove_from_bucket(bucket, map->elem_size, key, i);
            map->count --;
        }
    } 
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
    ecs_assert(map != NULL, ECS_INVALID_PARAMETER);
    clear_buckets(map);
    map->count = 0;
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
    if (!map) {
        return NULL;
    }
    
    ecs_assert(!elem_size || elem_size == map->elem_size, ECS_INVALID_PARAMETER);
 
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
    ecs_assert(map != NULL, ECS_INVALID_PARAMETER);
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
    ecs_assert(map != NULL, ECS_INVALID_PARAMETER);
    int32_t bucket_count = get_bucket_count(element_count);

    if (bucket_count) {
        rehash(map, bucket_count);
    }
}

void ecs_map_memory(
    ecs_map_t *map, 
    int32_t *allocd,
    int32_t *used)
{
    ecs_assert(map != NULL, ECS_INVALID_PARAMETER);

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

int8_t ecs_to_i8(
    int64_t v)
{
    ecs_assert(v < INT8_MAX, ECS_INTERNAL_ERROR);
    return (int8_t)v;
}

int16_t ecs_to_i16(
    int64_t v)
{
    ecs_assert(v < INT16_MAX, ECS_INTERNAL_ERROR);
    return (int16_t)v;
}

uint32_t ecs_to_u32(
    uint64_t v)
{
    ecs_assert(v < UINT32_MAX, ECS_INTERNAL_ERROR);
    return (uint32_t)v;    
}

int32_t ecs_to_i32(
    uint64_t v)
{
    ecs_assert(v < INT32_MAX, ECS_INTERNAL_ERROR);
    return (int32_t)v;      
}

size_t ecs_to_size_t(
    int64_t size)
{
    ecs_assert(size >= 0, ECS_INTERNAL_ERROR);
    return (size_t)size;
}

ecs_size_t ecs_from_size_t(
    size_t size)
{
   ecs_assert(size < INT32_MAX, ECS_INTERNAL_ERROR); 
   return (ecs_size_t)size;
}

int32_t ecs_next_pow_of_2(
    int32_t n)
{
    n --;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n ++;

    return n;
}

/** Convert time to double */
double ecs_time_to_double(
    ecs_time_t t)
{
    double result;
    result = t.sec;
    return result + (double)t.nanosec / (double)1000000000;
}

ecs_time_t ecs_time_sub(
    ecs_time_t t1,
    ecs_time_t t2)
{
    ecs_time_t result;

    if (t1.nanosec >= t2.nanosec) {
        result.nanosec = t1.nanosec - t2.nanosec;
        result.sec = t1.sec - t2.sec;
    } else {
        result.nanosec = t1.nanosec - t2.nanosec + 1000000000;
        result.sec = t1.sec - t2.sec - 1;
    }

    return result;
}

void ecs_sleepf(
    double t)
{
    if (t > 0) {
        int sec = (int)t;
        int nsec = (int)((t - sec) * 1000000000);
        ecs_os_sleep(sec, nsec);
    }
}

double ecs_time_measure(
    ecs_time_t *start)
{
    ecs_time_t stop, temp;
    ecs_os_get_time(&stop);
    temp = stop;
    stop = ecs_time_sub(stop, *start);
    *start = temp;
    return ecs_time_to_double(stop);
}

void* ecs_os_memdup(
    const void *src, 
    ecs_size_t size) 
{
    if (!src) {
        return NULL;
    }
    
    void *dst = ecs_os_malloc(size);
    ecs_assert(dst != NULL, ECS_OUT_OF_MEMORY);
    ecs_os_memcpy(dst, src, size);  
    return dst;  
}

/*
    This code was taken from sokol_time.h 
    
    zlib/libpng license
    Copyright (c) 2018 Andre Weissflog
    This software is provided 'as-is', without any express or implied warranty.
    In no event will the authors be held liable for any damages arising from the
    use of this software.
    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:
        1. The origin of this software must not be misrepresented; you must not
        claim that you wrote the original software. If you use this software in a
        product, an acknowledgment in the product documentation would be
        appreciated but is not required.
        2. Altered source versions must be plainly marked as such, and must not
        be misrepresented as being the original software.
        3. This notice may not be removed or altered from any source
        distribution.
*/


static int ecs_os_time_initialized;

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
static double _ecs_os_time_win_freq;
static LARGE_INTEGER _ecs_os_time_win_start;
#elif defined(__APPLE__) && defined(__MACH__)
#include <mach/mach_time.h>
static mach_timebase_info_data_t _ecs_os_time_osx_timebase;
static uint64_t _ecs_os_time_osx_start;
#else /* anything else, this will need more care for non-Linux platforms */
#include <time.h>
static uint64_t _ecs_os_time_posix_start;
#endif

/* prevent 64-bit overflow when computing relative timestamp
    see https://gist.github.com/jspohr/3dc4f00033d79ec5bdaf67bc46c813e3
*/
#if defined(_WIN32) || (defined(__APPLE__) && defined(__MACH__))
int64_t int64_muldiv(int64_t value, int64_t numer, int64_t denom) {
    int64_t q = value / denom;
    int64_t r = value % denom;
    return q * numer + r * numer / denom;
}
#endif

void ecs_os_time_setup(void) {
    if ( ecs_os_time_initialized) {
        return;
    }
    
    ecs_os_time_initialized = 1;
    #if defined(_WIN32)
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&_ecs_os_time_win_start);
        _ecs_os_time_win_freq = (double)freq.QuadPart / 1000000000.0;
    #elif defined(__APPLE__) && defined(__MACH__)
        mach_timebase_info(&_ecs_os_time_osx_timebase);
        _ecs_os_time_osx_start = mach_absolute_time();
    #else
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        _ecs_os_time_posix_start = (uint64_t)ts.tv_sec*1000000000 + (uint64_t)ts.tv_nsec; 
    #endif
}

uint64_t ecs_os_time_now(void) {
    ecs_assert(ecs_os_time_initialized != 0, ECS_INTERNAL_ERROR);

    uint64_t now;

    #if defined(_WIN32)
        LARGE_INTEGER qpc_t;
        QueryPerformanceCounter(&qpc_t);
        now = (uint64_t)(qpc_t.QuadPart / _ecs_os_time_win_freq);
    #elif defined(__APPLE__) && defined(__MACH__)
        now = mach_absolute_time();
    #else
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        now = ((uint64_t)ts.tv_sec * 1000000000 + (uint64_t)ts.tv_nsec);
    #endif

    return now;
}

void ecs_os_time_sleep(
    int32_t sec, 
    int32_t nanosec) 
{
#ifndef _WIN32
    struct timespec sleepTime;
    ecs_assert(sec >= 0, ECS_INTERNAL_ERROR);
    ecs_assert(nanosec >= 0, ECS_INTERNAL_ERROR);

    sleepTime.tv_sec = sec;
    sleepTime.tv_nsec = nanosec;
    if (nanosleep(&sleepTime, NULL)) {
        ecs_os_err("nanosleep failed");
    }
#else
    HANDLE timer;
    LARGE_INTEGER ft;

    ft.QuadPart = -((int64_t)sec * 10000000 + (int64_t)nanosec / 100);

    timer = CreateWaitableTimer(NULL, TRUE, NULL);
    SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
    WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);
#endif
}


#if defined(_WIN32)

static ULONG win32_current_resolution;

void ecs_increase_timer_resolution(bool enable)
{
    HMODULE hntdll = GetModuleHandle((LPCTSTR)"ntdll.dll");
    if (!hntdll) {
        return;
    }

    LONG (__stdcall *pNtSetTimerResolution)(
        ULONG desired, BOOLEAN set, ULONG * current);

    pNtSetTimerResolution = (LONG(__stdcall*)(ULONG, BOOLEAN, ULONG*))
        GetProcAddress(hntdll, "NtSetTimerResolution");

    if(!pNtSetTimerResolution) {
        return;
    }

    ULONG current, resolution = 10000; /* 1 ms */

    if (!enable && win32_current_resolution) {
        pNtSetTimerResolution(win32_current_resolution, 0, &current);
        win32_current_resolution = 0;
        return;
    } else if (!enable) {
        return;
    }

    if (resolution == win32_current_resolution) {
        return;
    }

    if (win32_current_resolution) {
        pNtSetTimerResolution(win32_current_resolution, 0, &current);
    }

    if (pNtSetTimerResolution(resolution, 1, &current)) {
        /* Try setting a lower resolution */
        resolution *= 2;
        if(pNtSetTimerResolution(resolution, 1, &current)) return;
    }

    win32_current_resolution = resolution;
}

#else
void ecs_increase_timer_resolution(bool enable)
{
    (void)enable;
    return;
}
#endif

/* Component lifecycle actions for EcsName */
static
void EcsName_ctor(EcsName *ptr, size_t size, int32_t count) {
    memset(ptr, 0, size * ecs_to_size_t(count));
}

static
void EcsName_dtor(EcsName *ptr, size_t size, int32_t count) {
    (void)size;

    int32_t i;
    for (i = 0; i < count; i ++) {
        ecs_os_free(ptr[i].alloc_value);
    }
}

static
void EcsName_copy(EcsName *dst, const EcsName *src, size_t size, int32_t count) {
    (void)size;
    
    int32_t i;
    for (i = 0; i < count; i ++) {
        if (dst[i].alloc_value) {
            ecs_os_free(dst[i].alloc_value);
            dst[i].alloc_value = NULL;
        }
        
        if (src[i].alloc_value) {
            dst[i].alloc_value = ecs_os_strdup(src[i].alloc_value);
            dst[i].value = dst[i].alloc_value;
        } else {
            dst[i].alloc_value = NULL;
            dst[i].value = src[i].value;
        }
        dst[i].symbol = src[i].symbol;
    }
}

static
void EcsName_move(EcsName *dst, EcsName *src, size_t size, int32_t count) {
    memcpy(dst, src, size * ecs_to_size_t(count));
    memset(src, 0, size * ecs_to_size_t(count));
}


/* -- Bootstrapping -- */

#define bootstrap_component(world, table, name)\
    _bootstrap_component(world, table, ecs_typeid(name), #name, sizeof(name))

static
void _bootstrap_component(
    ecs_world_t *world,
    ecs_table_t *table,
    ecs_entity_t entity,
    const char *id,
    ecs_size_t size)
{
    ecs_assert(table != NULL, ECS_INTERNAL_ERROR);

    ecs_data_t *data = ecs_table_get_or_create_data(table);
    ecs_assert(data != NULL, ECS_INTERNAL_ERROR);

    ecs_column_t *columns = data->columns;
    ecs_assert(columns != NULL, ECS_INTERNAL_ERROR);

    /* Create record in entity index */
    ecs_record_t *record = ecs_eis_ensure(world, entity);
    record->table = table;

    /* Insert row into table to store EcsType itself */
    int32_t index = ecs_table_append(world, table, data, entity, record, false);
    record->row = index + 1;

    /* Set size and id */
    EcsType *c_info = ecs_vector_first(columns[0].data, EcsType);
    EcsName *id_data = ecs_vector_first(columns[1].data, EcsName);
    
    c_info[index].size = size;
    id_data[index].value = &id[ecs_os_strlen("Ecs")]; /* Skip prefix */
    id_data[index].symbol = id;
    id_data[index].alloc_value = NULL;
}

/** Create type for component */
ecs_type_t ecs_bootstrap_type(
    ecs_world_t *world,
    ecs_entity_t entity)
{
    ecs_table_t *table = ecs_table_find_or_create(world, &(ecs_entities_t){
        .array = (ecs_entity_t[]){entity},
        .count = 1
    });

    ecs_assert(table != NULL, ECS_INTERNAL_ERROR);
    ecs_assert(table->type != NULL, ECS_INTERNAL_ERROR);

    return table->type;
}

/** Initialize component table. This table is manually constructed to bootstrap
 * flecs. After this function has been called, the builtin components can be
 * created. 
 * The reason this table is constructed manually is because it requires the size
 * of the EcsType and EcsName components, which haven't been created yet */
static
ecs_table_t* bootstrap_component_table(
    ecs_world_t *world)
{
    ecs_entity_t entities[] = {ecs_typeid(EcsType), ecs_typeid(EcsName), ecs_pair(EcsScope, EcsFlecsCore)};
    ecs_entities_t array = {
        .array = entities,
        .count = 3
    };

    ecs_table_t *result = ecs_table_find_or_create(world, &array);
    ecs_data_t *data = ecs_table_get_or_create_data(result);

    /* Preallocate enough memory for initial components */
    data->entities = ecs_vector_new(ecs_entity_t, EcsFirstUserComponentId);
    data->record_ptrs = ecs_vector_new(ecs_record_t*, EcsFirstUserComponentId);

    data->columns = ecs_os_malloc(sizeof(ecs_column_t) * 2);
    ecs_assert(data->columns != NULL, ECS_OUT_OF_MEMORY);

    data->columns[0].data = ecs_vector_new(EcsType, EcsFirstUserComponentId);
    data->columns[0].size = sizeof(EcsType);
    data->columns[1].data = ecs_vector_new(EcsName, EcsFirstUserComponentId);
    data->columns[1].size = sizeof(EcsName);

    result->column_count = 2;
    
    return result;
}

void ecs_bootstrap(
    ecs_world_t *world)
{
    ecs_trace_1("bootstrap core components");
    ecs_log_push();

    /* Initialize storages for bootstrap components */
    ecs_store_ensure(world, ecs_typeid(EcsType));
    ecs_store_ensure(world, ecs_typeid(EcsName));

    /* Create table that will hold components (EcsType, EcsName) */
    ecs_table_t *table = bootstrap_component_table(world);
    assert(table != NULL);

    bootstrap_component(world, table, EcsType);
    bootstrap_component(world, table, EcsName);

    world->stats.last_component_id = EcsFirstUserComponentId;
    world->stats.last_id = EcsFirstUserEntityId;

    ecs_set_scope(world, EcsFlecsCore);

    ecs_bootstrap_tag(world, EcsModule);
    ecs_bootstrap_tag(world, EcsDisabled);
    ecs_bootstrap_tag(world, EcsWildcard);
    ecs_bootstrap_tag(world, EcsScope);

    ecs_set_lifecycle(world, ecs_typeid(EcsName), &(ecs_lifecycle_t){
        .ctor = (ecs_xtor_t)EcsName_ctor,
        .dtor = (ecs_xtor_t)EcsName_dtor,
        .copy = (ecs_copy_t)EcsName_copy,
        .move = (ecs_move_t)EcsName_move
    });

    /* Initialize scopes */
    ecs_set(world, EcsFlecs, EcsName, {.value = "flecs"});
    ecs_add_id(world, EcsFlecs, EcsModule);
    ecs_set(world, EcsFlecsCore, EcsName, {.value = "core"});
    ecs_add_id(world, EcsFlecsCore, EcsModule);
    ecs_add_id(world, EcsFlecsCore, ecs_pair(EcsScope, EcsFlecs));

    ecs_set_scope(world, 0);

    ecs_log_pop();
}
