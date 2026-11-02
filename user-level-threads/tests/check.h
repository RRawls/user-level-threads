#ifndef CHECK_H
#define CHECK_H
#include <stdio.h>
#include <stdlib.h>

static int checks_failed = 0;

//Records failure, but continues so that a single run can capture all issues
#define CHECK(cond, msg) do { 
    if(!(cond)){ 
        printf("TEST FAILED: %s (%s:%d)\n", msg, __FILE__, __LINE__); 
        checks_failed++; 
    } 
} while(0)

#define TEST_DONE(name) do { 
    printf("%s: %s\n", name, checks_failed ? "FAIL" : "PASS");
    return checks_failed ? 1 : 0;
} while(0)
#endif
