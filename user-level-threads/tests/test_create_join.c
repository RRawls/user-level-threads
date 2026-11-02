/* Thread creation, return values, worker_exit, and join edge cases. */
#include "../thread-worker.h"
#include "check.h"

static void *ret_arg_times_two(void *a){
    return (void *)((long)a * 2); 
}

static void *exit_explicit(void *a){ 
    worker_exit((void *)99); 
    return (void *)1; //unreachable 
}

static void *ret_null(void *a){ 
    return NULL; 
}

static void *nested_child(void *a){ 
    return (void *)7; 
}
static void *nested_parent(void *a){
    worker_t t; void *r = NULL;
    worker_create(&t, NULL, nested_child, NULL);
    worker_join(t, &r);
    return (void *)((long)r + 1);
}

int main(void){
    worker_t t[5]; 
    void *r;

    //values passed in and returned out
    for(long i = 0; i < 5; i++) CHECK(worker_create(&t[i], NULL, ret_arg_times_two, (void *)i) == 0, "create");
    for(long i = 0; i < 5; i++){
        r = (void *)-1;
        CHECK(worker_join(t[i], &r) == 0, "join");
        CHECK((long)r == i * 2, "return value via join");
    }

    //thread ids are unique
    worker_t a, b;
    worker_create(&a, NULL, ret_null, NULL);
    worker_create(&b, NULL, ret_null, NULL);
    CHECK(a != b, "unique thread ids");
    worker_join(a, NULL); //NULL value_ptr must be accepted
    worker_join(b, NULL);

    //explicit worker_exit(value)
    worker_t e; 
    worker_create(&e, NULL, exit_explicit, NULL);
    r = NULL;
    worker_join(e, &r);
    CHECK((long)r == 99, "worker_exit value reaches join");

    //otherwise NULL value
    worker_t n;
    worker_create(&n, NULL, ret_null, NULL);
    r = (void *)-1;
    worker_join(n, &r);
    CHECK(r == NULL, "NULL return value");

    //nested create + join from inside a worker
    worker_t p;
    worker_create(&p, NULL, nested_parent, NULL);
    worker_join(p, &r);
    CHECK((long)r == 8, "nested create/join");

    //testing join edge cases 
    CHECK(worker_join(9999, NULL) == -1, "join of nonexistent thread fails");
    CHECK(worker_join(t[0], NULL) == -1, "second join of same thread fails");

    //joining a thread that already finished (ensures it has time to run first) 
    worker_t f;
    worker_create(&f, NULL, ret_arg_times_two, (void *)21);
    for(int i = 0; i < 3; i++) worker_yield();
    r = NULL;
    CHECK(worker_join(f, &r) == 0 && (long)r == 42, "join after thread already finished");

    TEST_DONE("test_create_join");
}
