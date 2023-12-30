#ifndef REGEX_NFA_H
#define REGEX_NFA_H

#include "dfa.h"
#include "nfa.h"

typedef struct regex regex_t;

regex_t* new_regex(char*);
void free_regex(regex_t*);
/**
 * Returns true if the regex accepts the provided string.
*/
bool regex_accepts(regex_t*, char*);

// Reads from stdin and returns an NFA.
nfa_t* regex_to_nfa();

#endif  // REGEX_NFA_H
