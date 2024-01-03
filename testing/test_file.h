#ifndef TEST_FILE_H
#define TEST_FILE_H

#include <dlfcn.h>
#include <stdio.h>

#include "test.h"

// Puglic API for test files
#define TEST_CASE(name) void name(void)
#define REGISTER_TEST(func) __plugin_register_test(#func, func, __FILE__)

#define assert_true(condition) __plugin_assert(condition, #condition, __FILE__, __func__, __LINE__);
#define assert_false(condition) \
   __plugin_assert(!(condition), #condition, __FILE__, __func__, __LINE__);
#define assert(condition) __plugin_assert(condition, #condition, __FILE__, __func__, __LINE__);

// Main executeble API references (private)
static register_test_f __main_register_test = NULL;
static assert_f __main_assert = NULL;

static void* __load_main_function(const char* symbol_name) {
   void* symbol = dlsym(RTLD_DEFAULT, symbol_name);
   if (symbol == NULL) {
      printf("[PLUGIN] %s not found\n", symbol_name);
      exit(EXIT_FAILURE);
   }
   return symbol;
}

static void __plugin_register_test(const char* name, test_func_t func, const char* filename) {
   if (__main_register_test == NULL) {
      __main_register_test = (register_test_f)__load_main_function("__register_test");
   }
   __main_register_test(name, func, filename);
}

static void __plugin_assert(bool condition, const char* condition_str, const char* file,
                            const char* func, int line) {
   if (__main_assert == NULL) {
      __main_assert = (assert_f)__load_main_function("__assert");
   }
   __main_assert(condition, condition_str, file, func, line);
}

#endif  // TEST_FILE_H
