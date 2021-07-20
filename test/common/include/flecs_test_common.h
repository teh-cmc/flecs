#ifndef COMMON_H
#define COMMON_H

/* This generated file contains includes for project dependencies */
#include "flecs-test-common/bake_config.h"

void install_test_abort();

#define ITER_MAX_ENTITIES (64)
#define ITER_MAX_TERMS (16)
#define ITER_MAX_VARIABLES (16)

typedef struct ecs_iter_result_t {
    ecs_entity_t entities[ITER_MAX_ENTITIES];
    ecs_id_t term_ids[ITER_MAX_ENTITIES][ITER_MAX_TERMS];
    void *term_columns[ITER_MAX_TERMS];
  
    int32_t table_count;
    int32_t term_count;
    int32_t term_count_actual;

    int32_t _table_count;

    char *entity_names[ITER_MAX_ENTITIES];
    char *term_ids_expr[ITER_MAX_ENTITIES][ITER_MAX_TERMS];
    int32_t matched[ITER_MAX_ENTITIES];

    struct {
        int32_t id;
        ecs_entity_t entities[ITER_MAX_ENTITIES];
        char *entity_names[ITER_MAX_ENTITIES];
    } variables[ITER_MAX_VARIABLES];
} ecs_iter_result_t;

// Utility for doing order-independent validation of iterator output
FLECS_TEST_COMMON_API
bool test_iter(
    ecs_iter_t *it, 
    ecs_iter_next_action_t next, 
    ecs_iter_result_t *expect);

#endif

