#ifndef DFA_H
#define DFA_H

#include "list.h"
#include "nfa.h"

typedef struct dfa dfa_t;
typedef struct dfa_node dfa_node_t;
typedef struct dfa_edge dfa_edge_t;

struct dfa {
      dfa_node_t* start;
      list_t* __nodes;
};

struct dfa_node {
      char* id;
      bool is_accepting;
      list_t* edges;
};

struct dfa_edge {
      char value;
      dfa_node_t* to;
};

dfa_t* dfa_from_nfa(nfa_t* nfa);
void free_dfa(dfa_t* dfa);
void log_dfa(dfa_t* dfa);

#endif  // DFA_H
