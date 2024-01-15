/**
 * AK: Parse basic regex into an NFA structure
 * 
 * Regex EBNF grammar:
 * <regexp> -> <concat> { "|" <concat> }
 * <concat> -> <quantifier> { <quantifier> }
 * <quantifier> -> <factor> [ <quantifier-symbol> ]
 * <factor> ( <regexp> ) | Letter
 * <quantifier-symbol> -> * | + | ?
 * 
 * Inputs a line of text from stdin
 * Outputs "Error" or the result.
 * 
*/

#include "parse.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

/**
 * Improvement ideas:
 * - Instead of `start` on nfa be a node, make it an edge that points to the start node
 *   - this might make it easier to compose nfas
 * - Add support for "any character" (.)
 * - Add support for escape sequences
 * - Add support for character classes / ranges
 * - Expand set of recognized literals
 */

typedef struct state {
      const char* pattern;
      char* current;
} state_t;

static char peek(state_t*);
static void match(state_t*, char);

// Recursive descent functions
static nfa_t* regexp(state_t*);
static nfa_t* concat(state_t*);
static nfa_t* quantifier(state_t*);
static nfa_t* factor(state_t*);

// Constructors per parser grammar
static nfa_t* new_choice_nfa(nfa_t*, nfa_t*);      // 'a|b'
static nfa_t* new_concat_nfa(nfa_t*, nfa_t*);      // 'ab'
static nfa_t* new_repetition_nfa(nfa_t*);          // 'a*'
static nfa_t* new_min_one_repetition_nfa(nfa_t*);  // 'a+'
static nfa_t* new_optional_nfa(nfa_t*);            // 'a?'
static nfa_t* new_literal_nfa(char);               // 'a'

// Don't need to escape single quotes
const char ESCAPABLE_CHARACTERS[] = {
    '*', '+', '?', '.', '|', '(', ')', '\\', '/', '"', 't', 'n', 'r',
};

static bool valid_literal(char c) { return isalnum(c) || c == ' ' || c == '\''; }

static bool is_quantifier_symbol(char c) { return c == '*' || c == '+' || c == '?'; }

static bool is_escapable_character(char c) {
   for (int i = 0; i < sizeof(ESCAPABLE_CHARACTERS); i++) {
      if (c == ESCAPABLE_CHARACTERS[i]) {
         return true;
      }
   }
   return false;
}

static char get_escaped_character(char c) {
   switch (c) {
      case 't':
         return '\t';
      case 'n':
         return '\n';
      case 'r':
         return '\r';
      default:
         return c;
   }
}

// Whether the character is in the first set of 'factor'
static bool in_factor_first_set(char c) { return valid_literal(c) || c == '(' || c == '\\'; }

/**
 * Parser
*/

nfa_t* parse_regex_to_nfa(char* pattern) {
   state_t state = {
       .pattern = pattern,
       .current = pattern,
   };
   nfa_t* result = regexp(&state);

   if (peek(&state) != '\0') {
      error("Expected end of input (\'\\0\')");
   }

   return result;
}

static char peek(state_t* state) { return *state->current; }

static void match(state_t* state, char expectedToken) {
   if (peek(state) == expectedToken)
      state->current++;
   else
      error("Unexpected token");
}

static char next(state_t* state) {
   char c = peek(state);
   state->current++;
   return c;
}

static nfa_t* regexp(state_t* state) {
   nfa_t* temp = concat(state);
   while (peek(state) == '|') {
      match(state, '|');
      temp = new_choice_nfa(temp, concat(state));
   }
   return temp;
}

static nfa_t* concat(state_t* state) {
   nfa_t* temp = quantifier(state);
   while (in_factor_first_set(peek(state))) {
      // We don't match here since current token is part of first set of `factor`, so if we matched the conditions in factor will fail
      temp = new_concat_nfa(temp, quantifier(state));
   }
   return temp;
}

static nfa_t* quantifier(state_t* state) {
   nfa_t* temp = factor(state);
   if (is_quantifier_symbol(peek(state))) {
      char symbol = next(state);
      switch (symbol) {
         case '*':
            temp = new_repetition_nfa(temp);
            break;
         case '+':
            temp = new_min_one_repetition_nfa(temp);
            break;
         case '?':
            temp = new_optional_nfa(temp);
            break;
         default:
            error("Invalid quantifier symbol");
      }
   }
   return temp;
}

static nfa_t* factor(state_t* state) {
   nfa_t* temp = NULL;
   if (peek(state) == '(') {
      match(state, '(');
      temp = regexp(state);
      match(state, ')');
   } else if (peek(state) == '\\') {
      match(state, '\\');
      char value = next(state);
      if (!is_escapable_character(value)) {
         error("[factor] Invalid escapable character");
      }
      temp = new_literal_nfa(get_escaped_character(value));
   } else if (valid_literal(peek(state))) {
      char value = next(state);
      temp = new_literal_nfa(value);
   } else {
      error("[factor] Unexpected token");
   }
   return temp;
}

/**
 * NFA constructors
*/

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

static nfa_t* new_min_one_repetition_nfa(nfa_t* old_nfa) {
   nfa_t* nfa = new_nfa();
   nfa_consume_nodes(nfa, old_nfa);

   // Turn off 'accepting' of previous nfa
   old_nfa->end->is_accepting = false;

   // Create the start and end nodes of repetition nfa
   nfa_node_t* start_node = nfa_new_node(nfa, 1);
   nfa_node_t* end_node = nfa_new_node(nfa, 0);

   // Initialize epsilon edges for start node
   init_epsilon(&start_node->edges[0]);
   start_node->edges[0].to = old_nfa->start;

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

static nfa_t* new_optional_nfa(nfa_t* old_nfa) {
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
   nfa_edge_t* old_end_edges = new_edges(1);
   init_epsilon(old_end_edges);
   old_end_edges->to = end_node;
   node_set_edges(old_nfa->end, old_end_edges, 1);

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
