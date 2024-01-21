#ifndef DFA_H
#define DFA_H

#include <stdbool.h>

#include "list.h"
#include "nfa.h"

typedef struct dfa dfa_t;
typedef struct dfa_node dfa_node_t;
typedef struct dfa_edge dfa_edge_t;

bool dfa_accepts(dfa_t* dfa, char* str, int len);
dfa_t* dfa_from_nfa(nfa_t* nfa);
void free_dfa(dfa_t* dfa);
void log_dfa(dfa_t* dfa);

#endif  // DFA_H
