#ifndef REGEX_NFA_H
#define REGEX_NFA_H

#include "dfa.h"
#include "nfa.h"

typedef struct regex regex_t;

regex_t* new_regex(char*);
void regex_release(regex_t*);
/**
 * Returns true if the regex accepts the provided string.
*/
bool regex_accepts(regex_t*, char*);

#endif  // REGEX_NFA_H
