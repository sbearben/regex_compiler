/**
 * AK: Parse basic regex into an NFA structure
 * 
 * Regex EBNF grammar:
 * <regexp> -> <concat> { "|" <concat> }
 * <concat> -> <quantifier> { <quantifier> }
 * <quantifier> -> <factor> [ <quantifier-symbol> ]
 * <factor> ( <regexp> ) | Letter | \[<range>\]
 * <range> -> Letter [ - Letter ] <range>
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
#include <string.h>

#include "utils.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define LITERAL_START 32
#define LITERAL_END 126
#define ASCII_SIZE 128

const int NUM_LITERALS = LITERAL_END - LITERAL_START + 1;

/**
 * Improvement ideas:
 * - Instead of `start` on nfa be a node, make it an edge that points to the start node
 *   - this might make it easier to compose nfas
 * - Add support for character classes / ranges
 */

typedef struct state {
      const char* pattern;
      char* current;
} state_t;

typedef enum CharacterClass { ANY } CharClass_t;

typedef enum ChracterConfigColumn {
   VALID_CHARACTER = 0,
   SPECIAL_CHARACTER = 1,
   QUANTIFIER_SYMBOL = 2,
} CCCol_t;

static char peek(state_t*);
static void match(state_t*, char);

// Recursive descent functions
static nfa_t* regexp(state_t*);
static nfa_t* concat(state_t*);
static nfa_t* quantifier(state_t*);
static nfa_t* factor(state_t*);
static nfa_t* range(state_t*);

// Constructors per parser grammar
static nfa_t* new_choice_nfa(nfa_t*, nfa_t*);      // 'a|b'
static nfa_t* new_concat_nfa(nfa_t*, nfa_t*);      // 'ab'
static nfa_t* new_repetition_nfa(nfa_t*);          // 'a*'
static nfa_t* new_min_one_repetition_nfa(nfa_t*);  // 'a+'
static nfa_t* new_optional_nfa(nfa_t*);            // 'a?'
static nfa_t* new_literal_nfa(char);               // 'a'

// Chracter classes
static nfa_t* new_character_class(CharClass_t);
static nfa_t* new_nfa_from_character_set(char*);

// Character helpers
static int get_character_config(char, CCCol_t);
static int is_valid_character(char);
static int is_special_character(char);
static int is_quantifier_symbol(char);
static char get_escaped_character(char);
static bool in_factor_first_set(char);

/**
 * ascii table from 32 to 126 
 * Values represent (
 *    special_character: bool,
 *    quantifier_symbol: bool
 * )
*/
const int CHARACTER_CONFIG[][3] = {
    // Null character can cause problems, return a value that will fail a `== true` or `== false` check.
    {-1, -1, -1}, /* '\0' */
    {0, 0, 0},    /* 'SOH' */
    {0, 0, 0},    /* 'STX' */
    {0, 0, 0},    /* 'ETX' */
    {0, 0, 0},    /* 'EOT' */
    {0, 0, 0},    /* 'ENQ' */
    {0, 0, 0},    /* 'ACK' */
    {0, 0, 0},    /* 'BEL' */
    {0, 0, 0},    /* 'BS' */
    {1, 0, 0},    /* '\t' */
    {1, 0, 0},    /* '\n' */
    {0, 0, 0},    /* 'VT' */
    {0, 0, 0},    /* 'FF' */
    {1, 0, 0},    /* '\r' */
    {0, 0, 0},    /* 'SO' */
    {0, 0, 0},    /* 'SI' */
    {0, 0, 0},    /* 'DLE' */
    {0, 0, 0},    /* 'DC1' */
    {0, 0, 0},    /* 'DC2' */
    {0, 0, 0},    /* 'DC3' */
    {0, 0, 0},    /* 'DC4' */
    {0, 0, 0},    /* 'NAK' */
    {0, 0, 0},    /* 'SYN' */
    {0, 0, 0},    /* 'ETB' */
    {0, 0, 0},    /* 'CAN' */
    {0, 0, 0},    /* 'EM' */
    {0, 0, 0},    /* 'SUB' */
    {0, 0, 0},    /* 'ESC' */
    {0, 0, 0},    /* 'FS' */
    {0, 0, 0},    /* 'GS' */
    {0, 0, 0},    /* 'RS' */
    {0, 0, 0},    /* 'US' */
    {1, 0, 0},    /* ' ' */
    {1, 0, 0},    /* '!' */
    {1, 1, 0},    /* '"' */
    {1, 0, 0},    /* '#' */
    {1, 0, 0},    /* '$' */
    {1, 0, 0},    /* '%' */
    {1, 0, 0},    /* '&' */
    {1, 0, 0},    /* ''' */
    {1, 1, 0},    /* '(' */
    {1, 1, 0},    /* ')' */
    {1, 1, 1},    /* '*' */
    {1, 1, 1},    /* '+' */
    {1, 0, 0},    /* ',' */
    {1, 0, 0},    /* '-' */
    {1, 1, 0},    /* '.' */
    {1, 0, 0},    /* '/' */
    {1, 0, 0},    /* '0' */
    {1, 0, 0},    /* '1' */
    {1, 0, 0},    /* '2' */
    {1, 0, 0},    /* '3' */
    {1, 0, 0},    /* '4' */
    {1, 0, 0},    /* '5' */
    {1, 0, 0},    /* '6' */
    {1, 0, 0},    /* '7' */
    {1, 0, 0},    /* '8' */
    {1, 0, 0},    /* '9' */
    {1, 0, 0},    /* ':' */
    {1, 0, 0},    /* ';' */
    {1, 0, 0},    /* '<' */
    {1, 0, 0},    /* '=' */
    {1, 0, 0},    /* '>' */
    {1, 1, 1},    /* '?' */
    {1, 0, 0},    /* '@' */
    {1, 0, 0},    /* 'A' */
    {1, 0, 0},    /* 'B' */
    {1, 0, 0},    /* 'C' */
    {1, 0, 0},    /* 'D' */
    {1, 0, 0},    /* 'E' */
    {1, 0, 0},    /* 'F' */
    {1, 0, 0},    /* 'G' */
    {1, 0, 0},    /* 'H' */
    {1, 0, 0},    /* 'I' */
    {1, 0, 0},    /* 'J' */
    {1, 0, 0},    /* 'K' */
    {1, 0, 0},    /* 'L' */
    {1, 0, 0},    /* 'M' */
    {1, 0, 0},    /* 'N' */
    {1, 0, 0},    /* 'O' */
    {1, 0, 0},    /* 'P' */
    {1, 0, 0},    /* 'Q' */
    {1, 0, 0},    /* 'R' */
    {1, 0, 0},    /* 'S' */
    {1, 0, 0},    /* 'T' */
    {1, 0, 0},    /* 'U' */
    {1, 0, 0},    /* 'V' */
    {1, 0, 0},    /* 'W' */
    {1, 0, 0},    /* 'X' */
    {1, 0, 0},    /* 'Y' */
    {1, 0, 0},    /* 'Z' */
    {1, 1, 0},    /* '[' */
    {1, 1, 0},    /* '\' */
    {1, 1, 0},    /* ']' */
    {1, 0, 0},    /* '^' */
    {1, 0, 0},    /* '_' */
    {1, 0, 0},    /* '`' */
    {1, 0, 0},    /* 'a' */
    {1, 0, 0},    /* 'b' */
    {1, 0, 0},    /* 'c' */
    {1, 0, 0},    /* 'd' */
    {1, 0, 0},    /* 'e' */
    {1, 0, 0},    /* 'f' */
    {1, 0, 0},    /* 'g' */
    {1, 0, 0},    /* 'h' */
    {1, 0, 0},    /* 'i' */
    {1, 0, 0},    /* 'j' */
    {1, 0, 0},    /* 'k' */
    {1, 0, 0},    /* 'l' */
    {1, 0, 0},    /* 'm' */
    {1, 0, 0},    /* 'n' */
    {1, 0, 0},    /* 'o' */
    {1, 0, 0},    /* 'p' */
    {1, 0, 0},    /* 'q' */
    {1, 0, 0},    /* 'r' */
    {1, 0, 0},    /* 's' */
    {1, 0, 0},    /* 't' */
    {1, 0, 0},    /* 'u' */
    {1, 0, 0},    /* 'v' */
    {1, 0, 0},    /* 'w' */
    {1, 0, 0},    /* 'x' */
    {1, 0, 0},    /* 'y' */
    {1, 0, 0},    /* 'z' */
    {1, 0, 0},    /* '{' */
    {1, 1, 0},    /* '|' */
    {1, 0, 0},    /* '}' */
    {1, 0, 0},    /* '~' */
};

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
   if (peek(state) == expectedToken) {
      state->current++;
   } else {
      error("Unexpected token");
   }
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
   while (in_factor_first_set(peek(state)) == true) {
      // We don't match here since current token is part of first set of `factor`, so if we matched the conditions in factor will fail
      temp = new_concat_nfa(temp, quantifier(state));
   }
   return temp;
}

static nfa_t* quantifier(state_t* state) {
   nfa_t* temp = factor(state);
   if (is_quantifier_symbol(peek(state)) == true) {
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
      temp = new_literal_nfa(get_escaped_character(value));
   } else if (is_special_character(peek(state)) == false) {
      char value = next(state);
      temp = new_literal_nfa(value);
   } else if (peek(state) == '.') {
      match(state, '.');
      temp = new_character_class(ANY);
   } else if (peek(state) == '[') {
      match(state, '[');
      temp = range(state);
      match(state, ']');
   } else {
      error("[factor] Unexpected token");
   }
   return temp;
}

static nfa_t* range(state_t* state) {
   static char seen_characters[ASCII_SIZE];
   memset(seen_characters, 0, sizeof seen_characters);
   int max_character_count = 0;

   // TODO: - if negation is implemented ('^'), then we need to handle escaping it to match '^' literally
   //       - seems it only needs to be escaped if it is the first character in the range
   //       - seems anything can be escaped and it's just treated as a literal
   while (peek(state) != ']') {
      char start = next(state);
      if (peek(state) == '-') {
         match(state, '-');
         if (is_valid_character(peek(state)) == false) {
            error("Invalid character in range");
         }
         char end = next(state);
         if (start > end) {
            continue;
         }
         for (char c = start; c <= end; c++) {
            seen_characters[(int)c] = 1;
         }
         max_character_count += end - start + 1;
      } else {
         seen_characters[(int)start] = 1;
         max_character_count++;
      }
   }

   // Create nfa from seen characters
   char* characters = (char*)xmalloc((MIN(max_character_count, ASCII_SIZE) + 1) * sizeof(char));
   int ch_index = 0;
   for (int i = 0; i < sizeof seen_characters; i++) {
      if (seen_characters[i] == 1) {
         characters[ch_index++] = i;
      }
   }
   characters[ch_index] = '\0';

   nfa_t* temp = new_nfa_from_character_set(characters);
   free(characters);

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

/**
 * Character classes
*/

static nfa_t* new_character_class(CharClass_t cctype) {
   // '.' matches any single character except line terminators \n, \r (but includes \t)
   static char any_characters[NUM_LITERALS + 2] = {0};

   switch (cctype) {
      case ANY:
         if (any_characters[0] == 0) {
            int index_offset = 0;
            for (char cl = LITERAL_START; cl <= LITERAL_END; cl++) {
               any_characters[index_offset++] = cl;
            }
            any_characters[index_offset++] = '\t';
            any_characters[index_offset] = '\0';
         }
         return new_nfa_from_character_set(any_characters);
      default:
         error("[new_character_class] unexpected character class");
   }
   return NULL;
}

static nfa_t* new_nfa_from_character_set(char* characters) {
   // Over-allocate space for escaping characters and joining pipe characters
   static char ch_set_regex_str[LITERAL_END * 3] = {0};
   memset(ch_set_regex_str, 0, sizeof ch_set_regex_str);

   int write_index = 0;
   for (char* chptr = characters; *chptr != '\0'; chptr++) {
      if (is_special_character(*chptr) == true) {
         ch_set_regex_str[write_index++] = '\\';
      }
      ch_set_regex_str[write_index++] = *chptr;
      if (*(chptr + 1) != '\0') {
         ch_set_regex_str[write_index++] = '|';
      }
   }
   ch_set_regex_str[write_index] = '\0';

   return parse_regex_to_nfa(ch_set_regex_str);
}

/**
 * Character helpers
*/

static int get_character_config(char c, CCCol_t col) {
   if (c < 0 || c > LITERAL_END) {
      return -1;
   }
   return CHARACTER_CONFIG[(int)c][col];
}

static int is_valid_character(char c) { return get_character_config(c, VALID_CHARACTER); }

static int is_special_character(char c) { return get_character_config(c, SPECIAL_CHARACTER); }

static int is_quantifier_symbol(char c) { return get_character_config(c, QUANTIFIER_SYMBOL); }

// NOTE: I don't know if this is needed, but it's here for now
// Q: Why would a regex be written as '\\t' to match a tab instead of writing a tab character directly?
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
static bool in_factor_first_set(char c) {
   return (is_valid_character(c) == true && is_special_character(c) == false) || c == '(' ||
          c == '\\' || c == '.' || c == '[';
}
