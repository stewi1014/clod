#pragma once
#include <stdio.h>

bool has_init = false;

#define TEST_MAIN(test_func) \
bool init() {\
    fputs("running tests from " __FILE__ "\n", stderr);\
    return true;\
}\
typeof(test_func) *__test_func = test_func;\
int main(int argc, char **argv)

#define TEST_CASE(name, ...) {\
    if (!has_init) has_init = init();\
    if (__test_func(name, ##__VA_ARGS__))\
        fputs("fail " #name "\n", stderr);\
    else\
        fputs("✅pass " #name "\n", stderr);\
}

#define TEST_ASSERT(expr) \
    if (!(expr)) {\
        fputs("assertion failed: " #expr "\n", stderr);\
        return 1;\
    }
