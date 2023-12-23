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

typedef struct nfa {
      node_t* start;
      node_t* end;
} nfa_t;

typedef struct node {
      bool is_accepting;
      edge_t* edges;  // Array of edges connected to other nodes
      int num_edges;
} node_t;

typedef struct edge {
      char value;  // null character if is_epsilon is true
      bool is_epsilon;
      node_t* to;
} edge_t;

nfa_t* regexp(void);
nfa_t* concat(void);
nfa_t* repetition(void);
nfa_t* factor(void);

nfa_t* new_choice_nfa(nfa_t*, nfa_t*);
nfa_t* new_concat_nfa(nfa_t*, nfa_t*);
nfa_t* new_repetition_nfa(nfa_t*);
nfa_t* new_literal_nfa(char);

node_t* new_node(bool, int);
void init_epsilon(edge_t*);

void free_nfa(nfa_t*);
void free_node(node_t*);

void* xmalloc(size_t size) {
   void* ptr = malloc(size);
   if (!ptr) {
      fprintf(stderr, "malloc failed\n");
      exit(EXIT_FAILURE)
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
      next = new_concat_node(temp, repetition());
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
   node_t* start_node = (node_t*)xmalloc(sizeof(node_t));
   node_t* end_node = (node_t*)xmalloc(sizeof(node_t));

   // Create epsilon edges for start node
   edge_t** edges = (edge_t**)xmalloc(2 * sizeof(edge_t));
   for (int i = 0; i < 2; i++) {
      init_epsilon(edges[i]);
   }
   edges[0]->to = left->start;
   edges[1]->to = right->start;
   start_node->edges = edges;

   // Create epsilon edges that connect to end node
   edge_t** edges = (edge_t**)xmalloc(2 * sizeof(edge_t));
   for (int i = 0; i < 2; i++) {
      init_epsilon(edges[i]);
      edges[i]->to = end_node;
   }
   // TODO: come back to this as this might be a bug (since edges[0] references the two we allocated - ie do we need to make seperate allocations?)
   left->end->edges = edges[0];
   right->start->edges = edges[1];

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
   edge_t* edge = (edge_t*)xmalloc(1 * sizeof(edge_t));
   init_epsilon(edge);

   // Make the connection between the left and right side using the new 'edge'
   left->end->edges = edge;
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
   node_t* start_node = (node_t*)xmalloc(sizeof(node_t));
   node_t* end_node = (node_t*)xmalloc(sizeof(node_t));

   // Create epsilon edges for start node
   edge_t** edges = (edge_t**)xmalloc(2 * sizeof(edge_t));
   for (int i = 0; i < 2; i++) {
      init_epsilon(edges[i]);
   }
   edges[0]->to = old_nfa->start;
   edges[1]->to = end_node;
   start_node->edges = edges;

   // Create epsilon edges for old end node (could maybe reuse the edges alloced for start node?)
   edge_t** edges = (edge_t**)xmalloc(2 * sizeof(edge_t));
   for (int i = 0; i < 2; i++) {
      init_epsilon(edges[i]);
   }
   edges[0]->to = old_nfa->start;
   edges[1]->to = end_node;
   old_nfa->end->edges = edges;

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
   node_t* start_node = (node_t*)xmalloc(sizeof(node_t));
   node_t* end_node = (node_t*)xmalloc(sizeof(node_t));
   end_node->is_accepting = true;

   // Create the connecting edge with the literal value
   edge_t* edge = (edge_t*)xmalloc(1 * sizeof(edge_t));
   edge->value = value;
   edge->is_epsilon = false;

   // Make the connection between the left and right nodes using the new 'edge'
   start_node->edges = edge;
   edge->to = end_node;

   // Hook up start and end to nfa
   nfa->start = start_node;
   nfa->end = end_node;

   return nfa;
}

// Alloc a node and its edges -> edges are empty and need to be initialized
node_t* new_node(bool is_accepting, int num_edges) {
   node_t* node = (node_t*)xmalloc(sizeof(node_t));
   node->is_accepting = is_accepting;

   edge_t** edges = (edge_t**)xmalloc(num_edges * sizeof(edge_t));
   node->edges = edges;
   node->num_edges = num_edges;

   return node;
}

void init_epsilon(edge_t* edge) {
   edge->value = '\0';
   edge->is_epsilon = true;
}

void free_nfa(nfa_t* nfa) {
   free_node(nfa->start);
   free_node(nfa->end);
   free(nfa);
}

// TODO: this would probably result in infinite loop when there is a circular path in the NFA
void free_node(node_t* node) {
   edge_t* edge = node->edges;
   while (edge) {
      free_node(edge->to);
      free(edge);
      edge++;
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
   print_spaces(indentno);
   switch (tree->kind) {
      case Choice:
         printf("Choice\n");
         print_nfa(tree->child, indentno + 2);
         print_nfa(tree->rchild, indentno + 2);
         break;
      case Concat:
         printf("Concat\n");
         print_nfa(tree->child, indentno + 2);
         print_nfa(tree->rchild, indentno + 2);
         break;
      case Repetition:
         printf("Repetition\n");
         print_nfa(tree->child, indentno + 2);
         break;
      case Literal:
         printf("Literal: %c\n", tree->val);
         break;
      default:
         printf("Unknown node kind\n");
         break;
   }
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
