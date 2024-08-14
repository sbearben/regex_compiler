#ifndef NFA_H
#define NFA_H

#include "list.h"
#include "parse.h"

typedef struct nfa nfa_t;
typedef struct nfa_node nfa_node_t;
typedef struct nfa_edge nfa_edge_t;

struct nfa {
      nfa_node_t* start;
      nfa_node_t* end;
      // List of all nodes in this NFA
      list_t* __nodes;
      // Character set for this NFA
      char* __language;
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

/**
 * Creates an nfa from an ast.
 */
nfa_t* nfa_from_ast(ast_node_t*);

/**
 * Returns the number of states in the nfa.
 */
int nfa_num_states(nfa_t*);

/**
 * Returns the language of the nfa as a null-terminated string.
 */
char* nfa_language(nfa_t*);

/**
 * Finds the nfa_node a given nfa_node transtions to on a character.
 * @returns null if no nfa node with a transition on the character is found
 */
nfa_node_t* nfa_node_find_transition(nfa_node_t*, char);

/**
 * Frees the nfa.
 */
void free_nfa(nfa_t*);

/**
 * Logs the nfa to stdout.
 */
void log_nfa(nfa_t*);

#endif  // NFA_H
