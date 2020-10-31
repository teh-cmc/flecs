#include <base.h>

void Dense_setup() {
    ecs_os_set_api_defaults();
}

void Dense_ensure() {
    ecs_dense_t *d = ecs_dense_new(double);

    struct ecs_dense_ensure_get_t res = ecs_dense_ensure_get(d, double, 1);
    test_assert(res.ptr != NULL);
    test_bool(res.added, true);

    double *ptr = res.ptr;
    *ptr = 10.5;

    ptr = ecs_dense_get(d, double, 1);
    test_assert(ptr != NULL);
    test_flt(*ptr, 10.5);

    ecs_dense_free(d);
}

void Dense_ensure_exists() {
    ecs_dense_t *d = ecs_dense_new(double);

    struct ecs_dense_ensure_get_t res = ecs_dense_ensure_get(d, double, 1);
    test_assert(res.ptr != NULL); 
    test_bool(res.added, true);

    double *ptr = res.ptr;   
    *ptr = 10.5;

    test_assert(ptr == ecs_dense_ensure_get(d, double, 1).ptr);

    ecs_dense_free(d);
}

void Dense_get_not_exist() {
    ecs_dense_t *d = ecs_dense_new(double);

    test_assert(ecs_dense_get(d, double, 1) == NULL);

    ecs_dense_free(d);
}

void Dense_remove() {
    ecs_dense_t *d = ecs_dense_new(double);

    struct ecs_dense_ensure_get_t res = ecs_dense_ensure_get(d, double, 1);
    test_assert(res.ptr != NULL); 
    test_bool(res.added, true);
    
    test_assert(ecs_dense_get(d, double, 1) != NULL);

    ecs_dense_remove(d, 1);

    test_assert(ecs_dense_get(d, double, 1) == NULL);

    ecs_dense_free(d);
}

void Dense_remove_not_exists() {
    ecs_dense_t *d = ecs_dense_new(double);

    struct ecs_dense_ensure_get_t res = ecs_dense_ensure_get(d, double, 1);
    test_assert(res.ptr != NULL); 
    test_bool(res.added, true);
    
    test_assert(ecs_dense_get(d, double, 1) != NULL);

    ecs_dense_remove(d, 2);

    test_assert(ecs_dense_get(d, double, 1) != NULL);

    ecs_dense_free(d);
}

void Dense_get_0_size() {
    install_test_abort();

    ecs_dense_t *d = _ecs_dense_new(0);

    test_expect_abort();

    struct ecs_dense_ensure_get_t res = _ecs_dense_ensure_get(d, 0, 1);
    test_assert(res.ptr == NULL);
    test_bool(res.added, false);
}

void Dense_ensure_hi_id() {
    ecs_dense_t *d = ecs_dense_new(double);

    uint64_t id = ecs_pair(10, 1);

    bool added = ecs_dense_ensure(d, double, id);
    test_bool(added, true);

    double *ptr = ecs_dense_get(d, double, id);
    test_assert(ptr != NULL);

    ecs_dense_free(d);
}

void Dense_ensure_get_hi_id() {
    ecs_dense_t *d = ecs_dense_new(double);

    uint64_t id = ecs_pair(10, 1);

    struct ecs_dense_ensure_get_t res = ecs_dense_ensure_get(d, double, id);
    test_assert(res.ptr != NULL); 
    test_bool(res.added, true);    

    double *ptr = res.ptr;
    *ptr = 10.5;

    ptr = ecs_dense_get(d, double, id);
    test_assert(ptr != NULL);
    test_flt(*ptr, 10.5);

    ecs_dense_free(d);
}

void Dense_remove_hi_id() {
    ecs_dense_t *d = ecs_dense_new(double);

    uint64_t id = ecs_pair(10, 1);

    bool added = ecs_dense_ensure(d, double, id);
    test_bool(added, true);

    bool removed = ecs_dense_remove(d, id);
    test_bool(removed, true);

    double *ptr = ecs_dense_get(d, double, id);
    test_assert(ptr == NULL);

    ecs_dense_free(d);
}
