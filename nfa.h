#ifndef NFA_H
#define NFA_H

#include "list.h"

typedef struct nfa nfa_t;
// TODO: Rename to nfa_node_t
typedef struct nfa_node node_t;
// TODO: Rename to nfa_edge_t
typedef struct nfa_edge edge_t;

struct nfa {
      node_t* start;
      node_t* end;
      list_t* __nodes;
};

struct nfa_node {
      int id;
      bool is_accepting;
      edge_t* edges;  // Array of edges connected to other nodes (might be better as a linked list)
      int num_edges;
};

struct nfa_edge {
      char value;  // null character if is_epsilon is true
      bool is_epsilon;
      node_t* to;
};

// Generic constructors and setters
nfa_t* new_nfa();
void nfa_consume_nodes(nfa_t*, nfa_t*);
void nfa_set_start_end(nfa_t*, node_t*, node_t*);
node_t* nfa_new_node(nfa_t*, int);
node_t* new_node(int);
edge_t* new_edges(int);
void node_set_edges(node_t*, edge_t*, int);
void init_epsilon(edge_t*);

// Generic destructors
void free_nfa(nfa_t*);

// Traversal
typedef void (*on_node_f)(node_t*);
typedef void (*on_edge_f)(edge_t*);
void nfa_traverse(nfa_t*, on_node_f, on_edge_f);

// Logging
void log_nfa(nfa_t*);

#endif  // NFA_H
