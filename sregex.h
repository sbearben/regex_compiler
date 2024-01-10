// sregex (simple regex, not to be confused with structural regex)

#ifndef SREGEX_H
#define SREGEX_H

#include <stdbool.h>

typedef struct regex regex_t;

/**
 * Returns true if the regex accepts the provided string.
*/
bool regex_accepts(regex_t*, char*);

regex_t* new_regex(char*);
void regex_release(regex_t*);

#endif  // SREGEX_H
