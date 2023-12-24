/**
 * AK: Parse basic regex into an NFA structure
 * 
 * Regex EBNF grammar:
 * <regexp> -> <concat> { "|" <concat> }
 * <concat> -> <repetition> { <repetition> }
 * <repetition> -> <factor> [ * ]
 * <factor> ( <regexp> ) | Letter
 * 
 * Inputs a line of text from stdin
 * Outputs "Error" or the result.
 * 
*/

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "list.h"

// Holds the current input character for the parse
char token;

// typedef enum {
//    Choice,
//    Concat,
//    Repetition,
//    Literal,
// } node_kind;

typedef struct nfa nfa_t;
typedef struct node node_t;
typedef struct edge edge_t;

struct nfa {
      node_t* start;
      node_t* end;
      list_t* __nodes;
};

int node_id = 0;

struct node {
      int id;
      bool is_accepting;
      edge_t* edges;  // Array of edges connected to other nodes
      int num_edges;
      // Private - for freeing (since nodes can have circular references)
      bool __marked;
};

struct edge {
      char value;  // null character if is_epsilon is true
      bool is_epsilon;
      node_t* to;
};

// Recursive descent functions
nfa_t* regexp(void);
nfa_t* concat(void);
nfa_t* repetition(void);
nfa_t* factor(void);

// Constructors per parser grammar
nfa_t* new_choice_nfa(nfa_t*, nfa_t*);
nfa_t* new_concat_nfa(nfa_t*, nfa_t*);
nfa_t* new_repetition_nfa(nfa_t*);
nfa_t* new_literal_nfa(char);

// Generic constructors and setters
nfa_t* new_nfa();
void nfa_consume_nodes(nfa_t*, nfa_t*);
void nfa_set_start_end(nfa_t*, node_t*, node_t*);
node_t* nfa_new_node(nfa_t*, int);
// TODO: potentially delete this
node_t* new_node(int);
edge_t* new_edges(int);
void node_set_edges(node_t*, edge_t*, int);
void init_epsilon(edge_t*);

// Generic destructors
void free_nfa(nfa_t*);
void free_list_node(void*);
void nfa_traverse(nfa_t*, void (*on_node)(node_t*), void (*on_edge)(edge_t*));
void nodes_traverse(node_t*, void (*on_node)(node_t*), void (*on_edge)(edge_t*), list_t*);

// Logging
void log_nfa(nfa_t*, int);
void log_node(node_t*);
void log_edge(edge_t*);

void* xmalloc(size_t size) {
   void* ptr = malloc(size);
   if (!ptr) {
      fprintf(stderr, "malloc failed\n");
      exit(EXIT_FAILURE);
   }
   return ptr;
}

void error(void) {
   fprintf(stderr, "Error\n");
   exit(EXIT_FAILURE);
}

void match(char expectedToken) {
   if (token == expectedToken)
      token = getchar();
   else
      error();
}

nfa_t* regexp(void) {
   // TODO: for this, 'concat' and 'repitition' -> do we need two pointers?
   nfa_t *temp, *next;
   temp = concat();
   while (token == '|') {
      match('|');
      next = new_concat_nfa(temp, concat());
      temp = next;
   }
   return temp;
}

nfa_t* concat(void) {
   nfa_t *temp, *next;
   temp = repetition();
   while (isalnum(token) || token == '(') {
      // We don't match here since current token is part of first set of `factor`, so if we matched the conditions in factor will fail
      next = new_concat_nfa(temp, repetition());
      temp = next;
   }
   return temp;
}

nfa_t* repetition(void) {
   nfa_t *temp, *next;
   temp = factor();
   if (token == '*') {
      match('*');
      next = new_repetition_nfa(temp);
      temp = next;
   }
   return temp;
}

nfa_t* factor(void) {
   nfa_t* temp;
   if (token == '(') {
      match('(');
      temp = regexp();
      match(')');
   } else if (isalnum(token)) {
      char value = token;
      match(token);
      temp = new_literal_nfa(value);
   } else {
      error();
   }
   return temp;
}

nfa_t* new_choice_nfa(nfa_t* left, nfa_t* right) {
   nfa_t* nfa = new_nfa();

   nfa_consume_nodes(nfa, left);
   nfa_consume_nodes(nfa, right);

   // Turn off 'accepting' of both sides
   left->end->is_accepting = false;
   right->end->is_accepting = false;

   // Create the start and end nodes of choice nfa
   node_t* start_node = nfa_new_node(nfa, 2);
   node_t* end_node = nfa_new_node(nfa, 0);

   // Initialize epsilon edges for start node
   for (int i = 0; i < start_node->num_edges; i++) {
      init_epsilon(&start_node->edges[i]);
   }
   start_node->edges[0].to = left->start;
   start_node->edges[1].to = right->start;

   // Create epsilon edges that connect to end node
   edge_t* edges_to_end = new_edges(2);
   for (int i = 0; i < 2; i++) {
      init_epsilon(&edges_to_end[i]);
      edges_to_end[i].to = end_node;
   }
   node_set_edges(left->end, &edges_to_end[0], 1);
   node_set_edges(right->end, &edges_to_end[1], 1);

   // Hook up start and end to nfa
   nfa_set_start_end(nfa, start_node, end_node);

   // Free the old nfas, but not their contents
   free(left);
   free(right);

   return nfa;
}

nfa_t* new_concat_nfa(nfa_t* left, nfa_t* right) {
   nfa_t* nfa = new_nfa();
   nfa_consume_nodes(nfa, left);
   nfa_consume_nodes(nfa, right);

   // Turn off 'accepting' of both sides
   left->end->is_accepting = false;
   right->end->is_accepting = false;

   // Create the connecting epsilon edge
   edge_t* edges = new_edges(1);
   init_epsilon(edges);
   // Make the connection between the left and right side using the new 'edge'
   node_set_edges(left->end, edges, 1);
   edges->to = right->start;

   // Hook up start and end to nfa
   nfa_set_start_end(nfa, left->start, right->end);

   // Free the old nfas, but not their contents
   free(left);
   free(right);

   return nfa;
}

nfa_t* new_repetition_nfa(nfa_t* old_nfa) {
   nfa_t* nfa = new_nfa();
   nfa_consume_nodes(nfa, old_nfa);

   // Turn off 'accepting' of previous nfa
   old_nfa->end->is_accepting = false;

   // Create the start and end nodes of repetition nfa
   node_t* start_node = nfa_new_node(nfa, 2);
   node_t* end_node = nfa_new_node(nfa, 0);

   // Initialize epsilon edges for start node
   for (int i = 0; i < start_node->num_edges; i++) {
      init_epsilon(&start_node->edges[i]);
   }
   start_node->edges[0].to = old_nfa->start;
   start_node->edges[1].to = end_node;

   // Create epsilon edges for old end node
   edge_t* old_end_edges = new_edges(2);
   for (int i = 0; i < 2; i++) {
      init_epsilon(&old_end_edges[i]);
   }
   old_end_edges[0].to = old_nfa->start;
   old_end_edges[1].to = end_node;
   node_set_edges(old_nfa->end, old_end_edges, 2);

   // Hook up start and end to nfa
   nfa_set_start_end(nfa, start_node, end_node);

   // Free the old nfa, but not its contents;
   free(old_nfa);

   return nfa;
}

nfa_t* new_literal_nfa(char value) {
   nfa_t* nfa = new_nfa();

   // Create start and end nodes of literal nfa
   node_t* start_node = nfa_new_node(nfa, 1);
   node_t* end_node = nfa_new_node(nfa, 0);

   // Init connecting edge with the literal value and make connection
   start_node->edges[0].value = value;
   start_node->edges[0].is_epsilon = false;
   start_node->edges[0].to = end_node;

   // Hook up start and end to nfa
   nfa_set_start_end(nfa, start_node, end_node);

   return nfa;
}

nfa_t* new_nfa() {
   nfa_t* nfa = (nfa_t*)xmalloc(sizeof(nfa_t));
   nfa->start = NULL;
   nfa->end = NULL;

   nfa->__nodes = (list_t*)malloc(sizeof(list_t));
   list_initialize(nfa->__nodes, free_list_node);

   return nfa;
}

// Adds nodes from the 'other' nfa to target nfa and frees the 'other' nodes list (but not its elements whuch have been transfered)
void nfa_consume_nodes(nfa_t* nfa, nfa_t* other) {
   list_concat(nfa->__nodes, other->__nodes);
   free(other->__nodes);
}

// Set start and end node, and set end node to be accepting
void nfa_set_start_end(nfa_t* nfa, node_t* start, node_t* end) {
   nfa->start = start;
   nfa->end = end;
   nfa->end->is_accepting = true;
}

node_t* nfa_new_node(nfa_t* nfa, int num_edges) {
   node_t* node = new_node(num_edges);
   list_push(nfa->__nodes, node);

   return node;
}

// Alloc a node and its edges -> edges are empty and need to be initialized
node_t* new_node(int num_edges) {
   node_t* node = (node_t*)xmalloc(sizeof(node_t));
   node->id = node_id++;
   node->is_accepting = false;
   node->__marked = false;

   if (num_edges > 0) {
      edge_t* edges = new_edges(num_edges);
      node->edges = edges;
      node->num_edges = num_edges;
   } else {
      node->edges = NULL;
      node->num_edges = 0;
   }

   return node;
}

edge_t* new_edges(int num) {
   edge_t* edges = (edge_t*)xmalloc(num * sizeof(edge_t));
   for (int i = 0; i < num; i++) {
      edges[i].to = NULL;
   }

   return edges;
}

void node_set_edges(node_t* node, edge_t* edges, int num_edges) {
   // I believe it should always be the case that any node we are setting edges on didn't
   // have any edges previously - thus there is no need to free any previous edges.
   assert(node->edges == NULL);
   node->edges = edges;
   node->num_edges = num_edges;
}

void init_epsilon(edge_t* edge) {
   edge->value = '\0';
   edge->is_epsilon = true;
   edge->to = NULL;
}

void free_nfa(nfa_t* nfa) {
   // Releaase all nodes in the nfa
   list_release(nfa->__nodes);
   free(nfa);
}

void nfa_traverse(nfa_t* nfa, void (*on_node)(node_t*), void (*on_edge)(edge_t*)) {
   list_t* seen_nodes = (list_t*)malloc(sizeof(list_t));
   list_initialize(seen_nodes, NULL);

   nodes_traverse(nfa->start, on_node, on_edge, seen_nodes);

   free(seen_nodes);
}

void nodes_traverse(node_t* node, void (*on_node)(node_t*), void (*on_edge)(edge_t*),
                    list_t* seen_nodes) {
   if (list_contains(seen_nodes, node, NULL)) {
      return;
   }
   list_push(seen_nodes, node);
   if (on_node != NULL) {
      on_node(node);
   }

   for (int i = 0; i < node->num_edges; i++) {
      if (on_edge != NULL) {
         on_edge(&node->edges[i]);
      }
      nodes_traverse(node->edges[i].to, on_node, on_edge, seen_nodes);
   }
}

void free_list_node(void* data) {
   node_t* node = (node_t*)data;
   if (node->edges != NULL) {
      free(node->edges);
   }
   free(node);
}

void log_node(node_t* node) {
   printf("Node %d: %s\n", node->id, node->is_accepting ? "accepting" : "not accepting");
   for (int i = 0; i < node->num_edges; i++) {
      printf("\t");
      log_edge(&node->edges[i]);
   }
}

void log_edge(edge_t* edge) {
   if (edge->is_epsilon) {
      printf("Edge: epsilon\n");
   } else {
      printf("Edge: %c\n", edge->value);
   }
}

// Traverse nfa and log node/edge info
void log_nfa(nfa_t* nfa, int indentno) {
   printf("NFA:\n");
   nfa_traverse(nfa, log_node, NULL);
}

int main() {
   // load token with first character for lookahead
   token = getchar();
   nfa_t* result = regexp();
   // newline signals successful parse
   if (token == '\n') {
      log_nfa(result, 2);
      // char input[256];
      // printf("Enter string to match: ");
      // fgets(input, 256, stdin);
   } else {
      // extraneous chars on line
      error();
   }

   free_nfa(result);

   return 0;
}
