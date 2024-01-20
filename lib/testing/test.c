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

struct test_runner {
      list_t* test_cases;              // list of test_case_t
      test_case_t* current_test_case;  // used to easily associate assertions with test cases
      bool has_failures;
      // Test file
      const char* test_name;
      on_register_tests_f on_register_tests;
      void (*__dl_test_handle)(void);
};

struct test_case {
      const char* name;
      test_func_t func;
      const char* filename;
      list_t* test_results;  // list of test_result_t
};

struct test_result {
      const char* condition_str;
      int lineno;
      bool passed;  // right now just storing failed tests
};

static test_runner_t* runner = NULL;

static test_runner_t* test_runner_new(const char*);
static void test_run_all(test_runner_t*);

static void test_runner_release(test_runner_t*);
static void test_case_release(void*);

void __register_test(const char* name, test_func_t func, const char* filename) {
   test_case_t* test_case = (test_case_t*)malloc(sizeof(test_case_t));
   test_case->name = name;
   test_case->func = func;
   test_case->filename = filename;

   // Test results
   test_case->test_results = (list_t*)malloc(sizeof(list_t));
   list_initialize(test_case->test_results, NULL);

   list_push(runner->test_cases, test_case);
}

void __assert(bool condition, const char* condition_str, const char* file, const char* func,
              int line) {
   if (!condition) {
      runner->has_failures = true;

      test_result_t* test_result = (test_result_t*)malloc(sizeof(test_result_t));
      test_result->condition_str = condition_str;
      test_result->lineno = line;
      test_result->passed = false;

      list_push(runner->current_test_case->test_results, test_result);
   }
}

static test_runner_t* test_runner_new(const char* test_name) {
   test_runner_t* test_runner = (test_runner_t*)malloc(sizeof(test_runner_t));
   test_runner->test_cases = (list_t*)malloc(sizeof(list_t));
   list_initialize(test_runner->test_cases, test_case_release);
   test_runner->current_test_case = NULL;
   test_runner->has_failures = false;

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
   test_runner->__dl_test_handle = test_handle;

   return test_runner;
}

static void test_runner_release(test_runner_t* test_runner) {
   list_release(test_runner->test_cases);
   dlclose(test_runner->__dl_test_handle);
   free(test_runner);
}

static void test_case_release(void* data) {
   test_case_t* test_case = (test_case_t*)data;
   list_release(test_case->test_results);
   free(test_case);
}

static void test_run_all(test_runner_t* runner) {
   list_node_t* current_test_case;
   list_traverse(runner->test_cases, current_test_case) {
      test_case_t* test_case = (test_case_t*)current_test_case->data;
      printf("Running test: %s\n\n", test_case->name);
      runner->current_test_case = test_case;
      test_case->func();
      runner->current_test_case = NULL;
   }

   if (runner->has_failures) {
      printf("\nTest failures:\n");
      list_traverse(runner->test_cases, current_test_case) {
         test_case_t* test_case = (test_case_t*)current_test_case->data;
         printf("\n%s (file: %s)\n", test_case->name, test_case->filename);

         list_node_t* current_result;
         list_traverse(test_case->test_results, current_result) {
            test_result_t* test_result = (test_result_t*)current_result->data;
            if (!test_result->passed) {
               printf("  Assertion failed: %s\n", test_result->condition_str);
               printf("    File: %s\n", test_case->filename);
               printf("    Test: %s\n", test_case->name);
               printf("    Line: %d\n", test_result->lineno);
            }
         }
      }
      printf("\n");
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
