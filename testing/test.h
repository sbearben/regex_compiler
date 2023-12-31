#ifndef TEST_H
#define TEST_H

#include <stdbool.h>

#define TEST_CASE(name) void name(void)
#define REGISTER_TEST(func) __register_test(#func, func)

#define assert_true(condition) ___assert(condition, #condition, __FILE__, __func__, __LINE__);
#define assert_false(condition) ___assert(!(condition), #condition, __FILE__, __func__, __LINE__);
#define assert(condition) ___assert(condition, #condition, __FILE__, __func__, __LINE__);

typedef void (*test_func_t)(void);
typedef void (*on_register_tests_f)(void);

void __register_test(const char*, test_func_t);
void ___assert(bool, const char*, const char*, const char*, int);

#endif  // TEST_H
