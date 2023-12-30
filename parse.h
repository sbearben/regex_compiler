#ifndef PARSE_H
#define PARSE_H

#include "nfa.h"

// Parse null-terminated regex string into an NFA.
nfa_t* parse_regex_to_nfa(char*);

#endif  // PARSE_H
