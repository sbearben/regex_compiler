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

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

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
};

struct node {
      bool is_accepting;
      edge_t** edges;  // Array of edges connected to other nodes
      int num_edges;
};

struct edge {
      char value;  // null character if is_epsilon is true
      bool is_epsilon;
      node_t* to;
};

nfa_t* regexp(void);
nfa_t* concat(void);
nfa_t* repetition(void);
nfa_t* factor(void);

nfa_t* new_choice_nfa(nfa_t*, nfa_t*);
nfa_t* new_concat_nfa(nfa_t*, nfa_t*);
nfa_t* new_repetition_nfa(nfa_t*);
nfa_t* new_literal_nfa(char);

node_t* new_node(int);
edge_t** new_edges(int);
void set_node_edges(node_t*, edge_t**, int);
void init_epsilon(edge_t*);

void free_nfa(nfa_t*);
void free_node(node_t*);

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
   nfa_t* nfa = (nfa_t*)xmalloc(sizeof(nfa_t));

   // Turn off 'accepting' of both sides
   left->end->is_accepting = false;
   right->end->is_accepting = false;

   // Create the start and end nodes of choice nfa
   node_t* start_node = new_node(2);
   node_t* end_node = new_node(0);

   // Initialize epsilon edges for start node
   for (int i = 0; i < start_node->num_edges; i++) {
      init_epsilon(start_node->edges[i]);
   }
   start_node->edges[0]->to = left->start;
   start_node->edges[1]->to = right->start;

   // Create epsilon edges that connect to end node
   edge_t** edges_to_end = new_edges(2);
   for (int i = 0; i < 2; i++) {
      init_epsilon(edges_to_end[i]);
      edges_to_end[i]->to = end_node;
   }
   set_node_edges(left->end, &edges_to_end[0], 1);
   set_node_edges(right->start, &edges_to_end[1], 1);

   // Hook up start and end to nfas and adjust accepting
   nfa->start = start_node;
   nfa->end = end_node;
   nfa->end->is_accepting = true;

   // Free the old nfas, but not their contents
   free(left);
   free(right);

   return nfa;
}

nfa_t* new_concat_nfa(nfa_t* left, nfa_t* right) {
   nfa_t* nfa = (nfa_t*)xmalloc(sizeof(nfa_t));

   // Turn off 'accepting' of both sides
   left->end->is_accepting = false;
   right->end->is_accepting = false;

   // Create the connecting epsilon edge
   edge_t* edge = *new_edges(1);
   init_epsilon(edge);

   // Make the connection between the left and right side using the new 'edge'
   set_node_edges(left->end, &edge, 1);
   edge->to = right->start;

   // Hook up start and end to nfa and adjust accepting
   nfa->start = left->start;
   nfa->end = right->end;
   nfa->end->is_accepting = true;

   // Free the old nfas, but not their contents
   free(left);
   free(right);

   return nfa;
}

nfa_t* new_repetition_nfa(nfa_t* old_nfa) {
   nfa_t* nfa = (nfa_t*)xmalloc(sizeof(nfa_t));

   // Turn off 'accepting' of previous nfa
   old_nfa->end->is_accepting = false;

   // Create the start and end nodes of repetition nfa
   node_t* start_node = new_node(2);
   node_t* end_node = new_node(0);

   // Initialize epsilon edges for start node
   for (int i = 0; i < start_node->num_edges; i++) {
      init_epsilon(start_node->edges[i]);
   }
   start_node->edges[0]->to = old_nfa->start;
   start_node->edges[1]->to = end_node;

   // Create epsilon edges for old end node (could maybe reuse the edges alloced for start node?)
   edge_t** old_end_edges = new_edges(2);
   for (int i = 0; i < 2; i++) {
      init_epsilon(old_end_edges[i]);
   }
   old_end_edges[0]->to = old_nfa->start;
   old_end_edges[1]->to = end_node;
   set_node_edges(old_nfa->end, old_end_edges, 2);

   // Hook up start and end to nfa and adjust accepting
   nfa->start = start_node;
   nfa->end = end_node;
   nfa->end->is_accepting = true;

   // Free the old nfa, but not its contents;
   free(old_nfa);

   return nfa;
}

nfa_t* new_literal_nfa(char value) {
   nfa_t* nfa = (nfa_t*)xmalloc(sizeof(nfa_t));

   // Create start and end nodes of literal nfa
   node_t* start_node = new_node(1);
   node_t* end_node = new_node(0);
   end_node->is_accepting = true;

   // Init connecting edge with the literal value and make connection
   start_node->edges[0]->value = value;
   start_node->edges[0]->is_epsilon = false;
   start_node->edges[0]->to = end_node;

   // Hook up start and end to nfa
   nfa->start = start_node;
   nfa->end = end_node;

   return nfa;
}

// Alloc a node and its edges -> edges are empty and need to be initialized
node_t* new_node(int num_edges) {
   node_t* node = (node_t*)xmalloc(sizeof(node_t));
   node->is_accepting = false;

   if (num_edges > 0) {
      edge_t** edges = new_edges(num_edges);
      node->edges = edges;
      node->num_edges = num_edges;
   } else {
      node->edges = NULL;
      node->num_edges = 0;
   }

   return node;
}

edge_t** new_edges(int num) {
   edge_t** edges = (edge_t**)xmalloc(num * sizeof(edge_t));
   for (int i = 0; i < num; i++) {
      edges[i]->to = NULL;
   }

   return edges;
}

void set_node_edges(node_t* node, edge_t** edges, int num_edges) {
   // I believe it should always be the case that any node we are setting edges on didn't
   // have any edges previously - thus there is no need to free any previous edges.
   node->edges = edges;
   node->num_edges = num_edges;
}

void init_epsilon(edge_t* edge) {
   edge->value = '\0';
   edge->is_epsilon = true;
   edge->to = NULL;
}

void free_nfa(nfa_t* nfa) {
   free_node(nfa->start);
   free_node(nfa->end);
   free(nfa);
}

// TODO: this would probably result in infinite loop when there is a circular path in the NFA
void free_node(node_t* node) {
   for (int i = 0; i < node->num_edges; i++) {
      free_node(node->edges[i]->to);
      free(node->edges[i]);
   }
   free(node->edges);
   free(node);
}

static void print_spaces(int indentno) {
   for (int i = 0; i < indentno; i++) printf(" ");
}

/* procedure printTree prints a syntax tree to the 
 * listing file using indentation to indicate subtrees
 */
void print_nfa(nfa_t* nfa, int indentno) {
   // print_spaces(indentno);
   // switch (tree->kind) {
   //    case Choice:
   //       printf("Choice\n");
   //       print_nfa(tree->child, indentno + 2);
   //       print_nfa(tree->rchild, indentno + 2);
   //       break;
   //    case Concat:
   //       printf("Concat\n");
   //       print_nfa(tree->child, indentno + 2);
   //       print_nfa(tree->rchild, indentno + 2);
   //       break;
   //    case Repetition:
   //       printf("Repetition\n");
   //       print_nfa(tree->child, indentno + 2);
   //       break;
   //    case Literal:
   //       printf("Literal: %c\n", tree->val);
   //       break;
   //    default:
   //       printf("Unknown node kind\n");
   //       break;
   // }
}

int main() {
   // load token with first character for lookahead
   token = getchar();
   nfa_t* result = regexp();
   // newline signals successful parse
   if (token == '\n') {
      print_nfa(result, 2);
      // char input[256];
      // printf("Enter string to match: ");
      // fgets(input, 256, stdin);
   } else {
      // extraneous chars on line
      error();
   }

   // free_nfa(result);

   return 0;
}
