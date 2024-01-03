#include "utils.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

void* xmalloc(size_t size) {
   void* ptr = malloc(size);
   if (!ptr) {
      fprintf(stderr, "malloc failed\n");
      exit(EXIT_FAILURE);
   }
   return ptr;
}

int num_places(int n) {
   int r = 1;
   if (n < 0) n = -n;

   while (n > 9) {
      n /= 10;
      r++;
   }
   return r;
}

void error(char* msg) {
   fprintf(stderr, "Error: %s\n", msg);
   exit(EXIT_FAILURE);
}
