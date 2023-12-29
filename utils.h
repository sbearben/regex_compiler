#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>

void* xmalloc(size_t size);
void error(void);
// void* array_concat(void* array1, size_t size1, void* array2, size_t size2, size_t element_size);
int num_places(int n);

#endif  // UTILS_H
