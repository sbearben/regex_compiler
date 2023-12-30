#ifndef REGEX_H
#define REGEX_H

#include <stdbool.h>

typedef struct regex regex_t;

/**
 * Returns true if the regex accepts the provided string.
*/
bool regex_accepts(regex_t*, char*);

regex_t* new_regex(char*);
void regex_release(regex_t*);

#endif  // REGEX_H
