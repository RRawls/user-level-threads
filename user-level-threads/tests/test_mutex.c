/* Mutual exclusion, blocking, and wake-up. */
#include "../thread-worker.h"
#include "check.h"

#define THREADS 8
#define ITERS 50000

static worker_mutex_t m;
static long counter = 0;
static volatile int in_critical = 0;
static volatile int overlap = 0;

static void *incr(void *a){
    for(int i = 0; i < ITERS; i++){
        worker_mutex_lock(&m);
        counter++;
        worker_mutex_unlock(&m);
    }
    return NULL;
}

//yields while holding the lock; anyone who gets in at the same time is a bug
static void *hold_and_yield(void *a){
    for(int i = 0; i < 200; i++){
        worker_mutex_lock(&m);
        
        if (in_critical) {
            overlap = 1;
        }
        in_critical = 1;
        
        worker_yield();
        in_critical = 0;
        worker_mutex_unlock(&m);
    }
    return NULL;
}

int main(void){
    worker_t t[THREADS];

    CHECK(worker_mutex_init(&m, NULL) == 0, "mutex init");

    //lost-update check under preemption
    for (int i = 0; i < THREADS; i++) {
        worker_create(&t[i], NULL, incr, NULL);
    }
    for (int i = 0; i < THREADS; i++) {
        worker_join(t[i], NULL);
    }
    CHECK(counter == (long)THREADS * ITERS, "counter has no lost updates");

    //checks whether locks are held across a yield
    for (int i = 0; i < THREADS; i++) {
        worker_create(&t[i], NULL, hold_and_yield, NULL);
    }
    for (int i = 0; i < THREADS; i++) {
        worker_join(t[i], NULL);
    }
    CHECK(!overlap, "two threads never inside critical section together");

    //checks whether destroy behavior works as expectd (refuses while locked, succeeds when free)
    worker_mutex_lock(&m);
    CHECK(worker_mutex_destroy(&m) == -1, "destroy fails while locked");
    worker_mutex_unlock(&m);
    CHECK(worker_mutex_destroy(&m) == 0, "destroy succeeds when unlocked");

    //checks whether mutex can be re-initialized and reused 
    worker_mutex_init(&m, NULL);
    CHECK(worker_mutex_lock(&m) == 0 && worker_mutex_unlock(&m) == 0, "reuse after re-init");
    worker_mutex_destroy(&m);

    TEST_DONE("test_mutex");
}
