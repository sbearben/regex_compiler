#include "test.h"

#include <dlfcn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"

#define TEST_FILE_SUFFIX "_test.so"

typedef struct test_runner test_runner_t;
typedef struct test_case test_case_t;
typedef struct test_result test_result_t;
typedef struct test_file test_file_t;

struct test_runner {
      list_t* test_cases;  // list of test_case_t
      list_t* test_results;
      // Test file API
      const char* test_name;
      on_register_tests_f on_register_tests;
      void (*__test_handle)(void);
};

struct test_case {
      const char* name;
      test_func_t func;
};

struct test_result {
      const char* name;
      bool passed;
};

static test_runner_t* runner = NULL;

static test_runner_t* test_runner_new(const char*);
static void test_run_all(test_runner_t* runner);

void __register_test(const char* name, test_func_t func) {
   test_case_t* test_case = (test_case_t*)malloc(sizeof(test_case_t));
   test_case->name = name;
   test_case->func = func;

   list_push(runner->test_cases, test_case);
}

static test_runner_t* test_runner_new(const char* test_name) {
   test_runner_t* test_runner = (test_runner_t*)malloc(sizeof(test_runner_t));
   test_runner->test_cases = (list_t*)malloc(sizeof(list_t));
   list_initialize(test_runner->test_cases, NULL);
   // Test results
   // test_runner->test_results = (list_t*)malloc(sizeof(list_t));
   // list_initialize(test_runner->test_results, NULL);

   char test_file_name[strlen(test_name) + strlen(TEST_FILE_SUFFIX) + 1];
   strcpy(test_file_name, test_name);
   strcat(test_file_name, TEST_FILE_SUFFIX);

   void* test_handle = dlopen(test_file_name, RTLD_LAZY);
   if (test_handle == NULL) {
      printf("Failed to load test file: %s\n", test_file_name);
      exit(EXIT_FAILURE);
   }

   on_register_tests_f on_register_tests = dlsym(test_handle, "on_register_tests");
   if (on_register_tests == NULL) {
      printf("Failed to load on_register_tests function\n");
      exit(EXIT_FAILURE);
   }

   test_runner->test_name = test_name;
   test_runner->on_register_tests = on_register_tests;
   test_runner->__test_handle = test_handle;

   return test_runner;
}

static void test_runner_release(test_runner_t* test_runner) {
   list_release(test_runner->test_cases);
   // list_release(test_runner->test_results);
   dlclose(test_runner->__test_handle);
   free(test_runner);
}

static void test_run_all(test_runner_t* runner) {
   list_node_t* current;
   list_traverse(runner->test_cases, current) {
      test_case_t* test_case = (test_case_t*)current->data;
      printf("Running test: %s\n", test_case->name);
      test_case->func();
   }
}

void ___assert(bool condition, const char* condition_str, const char* file, const char* func,
               int line) {
   if (!condition) {
      printf("Assertion failed: %s\n", condition_str);
      printf("  File: %s\n", file);
      printf("  Func: %s\n", func);
      printf("  Line: %d\n", line);
      exit(EXIT_FAILURE);
   }
}

int main(int argc, char** argv) {
   if (argc < 2) {
      printf("Usage: %s <test>\n", argv[0]);
      return EXIT_FAILURE;
   }

   runner = test_runner_new(argv[1]);

   runner->on_register_tests();
   test_run_all(runner);

   test_runner_release(runner);

   return EXIT_SUCCESS;
}
