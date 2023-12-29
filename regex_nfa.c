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

#include "regex_nfa.h"

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

/**
 * Improvement ideas:
 * - Instead of `start` on nfa be a node, make it an edge that points to the start node
 *   - this might make it easier to compose nfas
 * - Add support for '?' and '+' operators
 * - Add support for character classes / ranges
 * - Add support for escape sequences
 */

// Holds the current input character for the parse
static char token;

// Recursive descent functions
static nfa_t* regexp(void);
static nfa_t* concat(void);
static nfa_t* repetition(void);
static nfa_t* factor(void);

// Constructors per parser grammar
static nfa_t* new_choice_nfa(nfa_t*, nfa_t*);
static nfa_t* new_concat_nfa(nfa_t*, nfa_t*);
static nfa_t* new_repetition_nfa(nfa_t*);
static nfa_t* new_literal_nfa(char);

nfa_t* regex_to_nfa() {
   // load token with first character for lookahead
   token = getchar();
   nfa_t* result = regexp();

   // newline signals successful parse
   if (token != '\n') {
      error();
   }

   return result;
}

static void match(char expectedToken) {
   if (token == expectedToken)
      token = getchar();
   else
      error();
}

static nfa_t* regexp(void) {
   nfa_t* temp = concat();
   while (token == '|') {
      match('|');
      temp = new_choice_nfa(temp, concat());
   }
   return temp;
}

static nfa_t* concat(void) {
   nfa_t* temp = repetition();
   while (isalnum(token) || token == '(') {
      // We don't match here since current token is part of first set of `factor`, so if we matched the conditions in factor will fail
      temp = new_concat_nfa(temp, repetition());
   }
   return temp;
}

static nfa_t* repetition(void) {
   nfa_t* temp = factor();
   if (token == '*') {
      match('*');
      temp = new_repetition_nfa(temp);
   }
   return temp;
}

static nfa_t* factor(void) {
   nfa_t* temp = NULL;
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

static nfa_t* new_choice_nfa(nfa_t* left, nfa_t* right) {
   nfa_t* nfa = new_nfa();
   nfa_consume_nodes(nfa, left);
   nfa_consume_nodes(nfa, right);

   // Turn off 'accepting' of both sides
   left->end->is_accepting = false;
   right->end->is_accepting = false;

   // Create the start and end nodes of choice nfa
   nfa_node_t* start_node = nfa_new_node(nfa, 2);
   nfa_node_t* end_node = nfa_new_node(nfa, 0);

   // Initialize epsilon edges for start node
   for (int i = 0; i < start_node->num_edges; i++) {
      init_epsilon(&start_node->edges[i]);
   }
   start_node->edges[0].to = left->start;
   start_node->edges[1].to = right->start;

   // Create epsilon edges that connect to end node
   // Left-end -> end
   nfa_edge_t* left_edge_to_end = new_edges(1);
   init_epsilon(left_edge_to_end);
   left_edge_to_end->to = end_node;
   node_set_edges(left->end, left_edge_to_end, 1);
   // Right-end -> end
   nfa_edge_t* right_edge_to_end = new_edges(1);
   init_epsilon(right_edge_to_end);
   right_edge_to_end->to = end_node;
   node_set_edges(right->end, right_edge_to_end, 1);

   // Hook up start and end to nfa
   nfa_set_start_end(nfa, start_node, end_node);

   // Free the old nfas, but not their contents
   free(left);
   free(right);

   return nfa;
}

static nfa_t* new_concat_nfa(nfa_t* left, nfa_t* right) {
   nfa_t* nfa = new_nfa();
   nfa_consume_nodes(nfa, left);
   nfa_consume_nodes(nfa, right);

   // Turn off 'accepting' of both sides
   left->end->is_accepting = false;
   right->end->is_accepting = false;

   // Create the connecting epsilon edge
   nfa_edge_t* edges = new_edges(1);
   init_epsilon(edges);
   // Make the connection between the left and right side using the new 'edge'
   edges->to = right->start;
   node_set_edges(left->end, edges, 1);

   // Hook up start and end to nfa
   nfa_set_start_end(nfa, left->start, right->end);

   // Free the old nfas, but not their contents
   free(left);
   free(right);

   return nfa;
}

static nfa_t* new_repetition_nfa(nfa_t* old_nfa) {
   nfa_t* nfa = new_nfa();
   nfa_consume_nodes(nfa, old_nfa);

   // Turn off 'accepting' of previous nfa
   old_nfa->end->is_accepting = false;

   // Create the start and end nodes of repetition nfa
   nfa_node_t* start_node = nfa_new_node(nfa, 2);
   nfa_node_t* end_node = nfa_new_node(nfa, 0);

   // Initialize epsilon edges for start node
   for (int i = 0; i < start_node->num_edges; i++) {
      init_epsilon(&start_node->edges[i]);
   }
   start_node->edges[0].to = old_nfa->start;
   start_node->edges[1].to = end_node;

   // Create epsilon edges for old end node
   nfa_edge_t* old_end_edges = new_edges(2);
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

static nfa_t* new_literal_nfa(char value) {
   nfa_t* nfa = new_nfa();

   // Create start and end nodes of literal nfa
   nfa_node_t* start_node = nfa_new_node(nfa, 1);
   nfa_node_t* end_node = nfa_new_node(nfa, 0);

   // Init connecting edge with the literal value and make connection
   start_node->edges[0].value = value;
   start_node->edges[0].is_epsilon = false;
   start_node->edges[0].to = end_node;

   // Hook up start and end to nfa
   nfa_set_start_end(nfa, start_node, end_node);

   return nfa;
}
