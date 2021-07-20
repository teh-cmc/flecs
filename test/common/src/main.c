#include <flecs_test_common.h>

void install_test_abort() {
    ecs_os_set_api_defaults();
    ecs_os_api_t os_api = ecs_os_api;
    os_api.abort_ = test_abort;
    ecs_os_set_api(&os_api);

    ecs_tracing_enable(-4);
}

int32_t find_entity(
    ecs_world_t *world,
    ecs_iter_result_t *expect, 
    ecs_entity_t e)
{
    int i;
    for (i = 0; i < ITER_MAX_ENTITIES; i ++) {
        if (expect->entities[i] == e) {
            while (expect->matched[i]) {
                i ++;
                if (!if_test_assert(e == expect->entities[i])) {
                    return -1;
                }
            }

            if (expect->entity_names[i]) {
                if (!if_test_str(ecs_get_name(world, e), expect->entity_names[i])) {
                    return -1;
                }
            }
            return i;
        }
    }

    for (i = 0; i < ITER_MAX_ENTITIES; i ++) {
        if (!expect->entity_names[i]) {
            break;
        }

        if (!strcmp(ecs_get_name(world, e), expect->entity_names[i])) {
            while (expect->matched[i]) {
                i ++;

                // If this fails, the entity is encountered more times than
                // expected.
                if (!if_test_str(ecs_get_name(world, e), 
                    expect->entity_names[i])) 
                {
                    return -1;
                }
            }

            return i;
        }
    }

    return -1;
}

bool test_iter(
    ecs_iter_t *it, 
    ecs_iter_next_action_t next, 
    ecs_iter_result_t *expect) 
{
    int32_t entity_index = -1;
    
    while (next(it)) {
        if (!if_test_int(it->term_count, expect->term_count)) {
            return false;
        }

        int32_t term_count_actual = expect->term_count_actual;
        if (!term_count_actual) {
            term_count_actual = it->term_count;
        }

        if (!if_test_int(it->term_count_actual, term_count_actual)) {
            return false;
        }

        // If actual term count is 0, shouldn't expect any non-zero term ids */
        if (!if_test_assert(term_count_actual || expect->term_ids[0][0] == 0)) {
            return false;
        }

        int i;
        for (i = 0; (i < it->count) || (i < 1); i ++) {
            ecs_entity_t e = 0;
            int t;

            if (it->count) {
                e = it->entities[i];
                entity_index = find_entity(it->world, expect, e);

                // Matched unexpected entity
                if (!if_test_assert(entity_index != -1)) {
                    return false;
                }

                expect->matched[entity_index] = true;

                // Test data
                for (t = 0; t < it->term_count_actual; t++) {
                    size_t size = ecs_term_size(it, t + 1);
                    if (!size) {
                        continue;
                    }

                    void *expect_ptr = expect->term_columns[t];
                    if (!expect_ptr) {
                        continue;
                    }

                    expect_ptr = ECS_OFFSET(expect_ptr, size * entity_index);

                    void *component_ptr = ecs_term_w_size(it, size, t + 1);
                    if (!if_test_assert(component_ptr != NULL)) {
                        return false;
                    }

                    component_ptr = ECS_OFFSET(component_ptr, size * i);
                    if (!if_test_assert(memcpy(component_ptr, expect_ptr, size))) {
                        return false;
                    }
                }                
            } else {
                entity_index ++;
            }


            // Test ids
            ecs_id_t *ids = expect->term_ids[entity_index];
            if (!ids[0]) {
                ids = expect->term_ids[0];
            }

            for (t = 0; t < it->term_count_actual; t++) {
                if (!ids[t]) {
                    break;
                }

                if (!if_test_assert(ecs_term_id(it, t + 1) == ids[t])) {
                    return false;
                }
            }

            if (!if_test_assert(ids[t] == 0)) {
                return false;
            }

            // Test ids by expr
            char **ids_expr = expect->term_ids_expr[entity_index];
            if (!ids_expr) {
                ids_expr = expect->term_ids_expr[0];
            }

            for (t = 0; t < it->term_count_actual; t++) {
                if (!ids_expr[t]) {
                    break;
                }

                char buf[256];
                ecs_id_str(it->world, ecs_term_id(it, t + 1), buf, 256);
                if (!if_test_str(buf, ids_expr[t])) {
                    printf(" - term %d\n", t);
                    if (e) {
                        printf(" - matched entity %u (%s, [%s])\n", 
                            (uint32_t)e,
                            ecs_get_name(it->world, e),
                            ecs_type_str(it->world, ecs_get_type(it->world, e)));

                        if (expect->entities[i]) {
                            printf(" - expected entity %u (%s)\n", 
                                (uint32_t)expect->entities[i], 
                                ecs_get_name(it->world, expect->entities[i]));
                        } else if (expect->entity_names[i]) {
                            printf(" - expected entity %s\n", 
                                expect->entity_names[i]);
                        }
                    }

                    printf(" - @ result index %d\n", entity_index);

                    return false;
                }
            }

            if (!if_test_assert(ids_expr[t] == NULL)) {
                return false;
            }
        }

        expect->_table_count ++;
    }

    for (int i = 0; i < ITER_MAX_ENTITIES; i ++) {
        if (expect->entities[i] || expect->entity_names[i]) {
            if (!if_test_assert(expect->matched[i])) {
                if (expect->entities[i]) {
                    printf(" - missing entity %u (index %d)\n", 
                        (uint32_t)expect->entities[i], i);
                } else {
                    printf(" - missing entity %s\n", expect->entity_names[i]);
                }
                return false;
            }
        }
    }

    if (expect->table_count) {
        if (!if_test_assert(expect->_table_count == expect->table_count)) {
            return false;
        }
    }

    return true;
}
