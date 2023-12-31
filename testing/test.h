#ifndef TEST_H
#define TEST_H

#include <stdbool.h>
#include <stdlib.h>

typedef void (*test_func_t)(void);
typedef void (*on_register_tests_f)(void);

typedef void (*register_test_f)(const char*, test_func_t);
typedef void (*assert_f)(bool, const char*, const char*, const char*, int);

#endif  // TEST_H
