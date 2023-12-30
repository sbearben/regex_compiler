#ifndef NFA_H
#define NFA_H

#include "list.h"

typedef struct nfa nfa_t;
typedef struct nfa_node nfa_node_t;
typedef struct nfa_edge nfa_edge_t;

struct nfa {
      nfa_node_t* start;
      nfa_node_t* end;
      list_t* __nodes;
};

struct nfa_node {
      int id;
      bool is_accepting;
      nfa_edge_t* edges;  // (might be better as a linked list)
      int num_edges;
};

struct nfa_edge {
      char value;  // null character if is_epsilon is true
      bool is_epsilon;
      nfa_node_t* to;
};

// Constructors and setters
nfa_t* new_nfa();
void nfa_consume_nodes(nfa_t*, nfa_t*);
void nfa_set_start_end(nfa_t*, nfa_node_t*, nfa_node_t*);
nfa_node_t* nfa_new_node(nfa_t*, int);
nfa_node_t* new_node(int);
nfa_edge_t* new_edges(int);
void node_set_edges(nfa_node_t*, nfa_edge_t*, int);
void init_epsilon(nfa_edge_t*);

// Destructors
void free_nfa(nfa_t*);

// Traversal
typedef void (*on_node_f)(nfa_node_t*);
void nfa_traverse(nfa_t*, on_node_f);

// Logging
void log_nfa(nfa_t*);

#endif  // NFA_H
