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

// void* array_concat(void* array1, size_t size1, void* array2, size_t size2, size_t element_size) {
//    void* array = xmalloc((size1 + size2) * element_size);
//    memcpy(array, array1, size1 * element_size);
//    memcpy(array + size1 * element_size, array2, size2 * element_size);

//    return array;
// }

int num_places(int n) {
   int r = 1;
   if (n < 0) n = -n;

   while (n > 9) {
      n /= 10;
      r++;
   }
   return r;
}

void error(void) {
   fprintf(stderr, "Error\n");
   exit(EXIT_FAILURE);
}
