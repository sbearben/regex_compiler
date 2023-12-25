#include "utils.h"

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

void error(void) {
   fprintf(stderr, "Error\n");
   exit(EXIT_FAILURE);
}
