#ifndef PARSE_H
#define PARSE_H

#include "nfa.h"

// Reads from stdin and returns an NFA.
nfa_t* parse_regex_to_nfa();

#endif  // PARSE_H
