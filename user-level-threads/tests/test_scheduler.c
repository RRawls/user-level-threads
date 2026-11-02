/* Round-robin order, voluntary yield, and timer preemption. */
#include "../thread-worker.h"
#include "check.h"

#define STEPS 5

static char log_buf[64];
static int log_len = 0;

static void *logger(void *a){
    char c = (char)(long)a;
    for(int i = 0; i < STEPS; i++){
        log_buf[log_len++] = c;
        worker_yield();
    }
    return NULL;
}

static volatile int flag = 0;
static void *spinner(void *a){ 
    while(!flag) { 
        //can only be stopped by the timer; never yields on its own
    } 
    return (void *)1; 
}

static void *setter(void *a){
    flag = 1; 
    return (void *)2; 
}

static volatile long progress[3];
static volatile int stop = 0;

static void *busy(void *a){ 
    long id = (long)a; 
    while(!stop) progress[id]++; 
    return NULL; 
}

static void *stopper(void *a){
    //burns some CPU resources so all busy threads get several time quantums, then stop them
    for(volatile long i = 0; i < 400000000L && !stop; i++){ 
        if (progress[0] && progress[1] && progress[2] && i > 1000000L) {
            break;
        }
    }
    stop = 1;
    return NULL;
}

int main(void){
    worker_t a, b, s1, s2;
    void *r;

    //strict alternation with two yielding threads (main is blocked in join)
    worker_create(&a, NULL, logger, (void *)'A');
    worker_create(&b, NULL, logger, (void *)'B');
    worker_join(a, NULL);
    worker_join(b, NULL);
    log_buf[log_len] = 0;
    CHECK(log_len == 2 * STEPS, "both threads ran all steps");
    
    int alternating = 1;
    for (int i = 0; i + 1 < log_len; i++) if (log_buf[i] == log_buf[i + 1]) {
        alternating = 0;
    }
    CHECK(alternating, "round robin alternates A/B on yield");

    //checks whether a thread that never yields still allows the setter to ge CPU time
    worker_create(&s1, NULL, spinner, NULL);
    worker_create(&s2, NULL, setter, NULL);
    worker_join(s1, &r);
    CHECK((long)r == 1, "spinner finished (timer preempted it)");
    worker_join(s2, NULL);

    //checks fairness by having 3 workers hog resources
    worker_t h[3], st;
    for (long i = 0; i < 3; i++) {
        worker_create(&h[i], NULL, busy, (void*)i);
    }
    
    worker_create(&st, NULL, stopper, NULL);
    worker_join(st, NULL);
    for (int i = 0; i < 3; i++) {
        worker_join(h[i], NULL);
    }
    CHECK(progress[0] > 0 && progress[1] > 0 && progress[2] > 0, "every busy thread got CPU time");

    TEST_DONE("test_scheduler");
}
