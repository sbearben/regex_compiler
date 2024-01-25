#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>

void* xmalloc(size_t);
void* xrealloc(void*, size_t);
void error(char*);
int num_places(int n);

#endif  // UTILS_H
