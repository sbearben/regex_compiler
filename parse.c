/**
 * AK: Parse basic regexes into an AST
 * 
 * Regex EBNF grammar:
 * <regexp> -> <concat> { "|" <concat> }
 * <concat> -> <quantifier> { <quantifier> }
 * <quantifier> -> <factor> [ <quantifier-symbol> ]
 * <factor> ( <regexp> ) | Letter | \[<negated-class-bracketed>\]
 * <negated-class-bracketed> -> [ ^ ] <class-bracketed>
 * <class-bracketed> -> Letter [ - Letter ] <class-bracketed>
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
 * - Add support for character classes
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
static ast_node_t* regexp(state_t*);
static ast_node_t* concat(state_t*);
static ast_node_t* quantifier(state_t*);
static ast_node_t* factor(state_t*);
static ast_node_t* class_bracketed(state_t*);

// Constructors for AST nodes
static ast_node_t* ast_new_option_node(ast_node_t*, ast_node_t*);         // 'a|b'
static ast_node_t* ast_new_concat_node(ast_node_t*, ast_node_t*);         // 'ab'
static ast_node_t* ast_new_repetition_node(RepetitionKind, ast_node_t*);  // 'a*|a+|a?'
static ast_node_t* ast_new_dot_node();                                    // '.'
static ast_node_t* ast_new_literal_node(char);                            // 'a'
static ast_node_t* ast_new_class_bracketed_node();                        // '[a-z]'

static class_set_item_t* class_bracketed_node_add_literal(node_class_bracketed_t*, char);
static class_set_item_t* class_bracketed_node_add_range(node_class_bracketed_t*, char, char);

// Character helpers
static int get_character_config(char, CCCol_t);
static int is_valid_character(char);
static int is_special_character(char);
static int is_quantifier_symbol(char);
static int in_factor_first_set(char);

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
    {1, 0, 0},    /* 'VT' */
    {1, 0, 0},    /* 'FF' */
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

ast_node_t* parse_regex(char* pattern) {
   state_t state = {
       .pattern = pattern,
       .current = pattern,
   };
   ast_node_t* result = regexp(&state);

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

static ast_node_t* regexp(state_t* state) {
   ast_node_t* temp = concat(state);
   while (peek(state) == '|') {
      match(state, '|');
      temp = ast_new_option_node(temp, concat(state));
   }
   return temp;
}

static ast_node_t* concat(state_t* state) {
   ast_node_t* temp = quantifier(state);
   while (in_factor_first_set(peek(state)) == true) {
      // We don't match here since current token is part of first set of `factor`, so if we matched the conditions in factor will fail
      temp = ast_new_concat_node(temp, quantifier(state));
   }
   return temp;
}

static ast_node_t* quantifier(state_t* state) {
   ast_node_t* temp = factor(state);
   if (is_quantifier_symbol(peek(state)) == true) {
      char symbol = next(state);
      RepetitionKind rep_kind;
      switch (symbol) {
         case '*':
            rep_kind = REPITITION_KIND_ZERO_OR_MORE;
            break;
         case '+':
            rep_kind = REPITITION_KIND_ONE_OR_MORE;
            break;
         case '?':
            rep_kind = REPITITION_KIND_ZERO_OR_ONE;
            break;
         default:
            error("Invalid quantifier symbol");
      }
      temp = ast_new_repetition_node(rep_kind, temp);
   }
   return temp;
}

static ast_node_t* factor(state_t* state) {
   ast_node_t* temp = NULL;
   if (peek(state) == '(') {
      match(state, '(');
      temp = regexp(state);
      match(state, ')');
   } else if (peek(state) == '\\') {
      match(state, '\\');
      char value = next(state);
      temp = ast_new_literal_node(value);
   } else if (is_special_character(peek(state)) == false) {
      char value = next(state);
      temp = ast_new_literal_node(value);
   } else if (peek(state) == '.') {
      match(state, '.');
      temp = ast_new_dot_node();
   } else if (peek(state) == '[') {
      match(state, '[');
      temp = range(state);
      match(state, ']');
   } else {
      error("[factor] Unexpected token");
   }
   return temp;
}

static ast_node_t* class_bracketed(state_t* state) {
   ast_node_t* temp = ast_new_class_bracketed_node();
   if (peek(state) == '^') {
      match(state, '^');
      temp->class_bracketed.negated = true;
   }

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
         class_bracketed_node_add_range(&temp->class_bracketed, start, end);
      } else {
         class_bracketed_node_add_literal(&temp->class_bracketed, start);
      }
   }

   return temp;
}

static ast_node_t* ast_new_option_node(ast_node_t* left, ast_node_t* right) {
   ast_node_t* node = (ast_node_t*)xmalloc(sizeof(ast_node_t));
   node->kind = NODE_KIND_OPTION;
   node->option.left = left;
   node->option.right = right;
   return node;
}

static ast_node_t* ast_new_concat_node(ast_node_t* left, ast_node_t* right) {
   ast_node_t* node = (ast_node_t*)xmalloc(sizeof(ast_node_t));
   node->kind = NODE_KIND_CONCAT;
   node->concat.left = left;
   node->concat.right = right;
   return node;
}

static ast_node_t* ast_new_repetition_node(RepetitionKind rep_kind, ast_node_t* child) {
   ast_node_t* node = (ast_node_t*)xmalloc(sizeof(ast_node_t));
   node->kind = NODE_KIND_REPITITION;
   node->repitition.rep_kind = rep_kind;
   node->repitition.child = child;
   return node;
}

static ast_node_t* ast_new_dot_node() {
   ast_node_t* node = (ast_node_t*)xmalloc(sizeof(ast_node_t));
   node->kind = NODE_KIND_DOT;
   return node;
}

static ast_node_t* ast_new_literal_node(char value) {
   ast_node_t* node = (ast_node_t*)xmalloc(sizeof(ast_node_t));
   node->kind = NODE_KIND_LITERAL;
   node->literal.value = value;
   return node;
}

static ast_node_t* ast_new_class_bracketed_node() {
   ast_node_t* node = (ast_node_t*)xmalloc(sizeof(ast_node_t));
   node->kind = NODE_KIND_CLASS_BRACKETED;
   node->class_bracketed.negated = false;
   node->class_bracketed.num_items = 0;
   node->class_bracketed.items = NULL;
   return node;
}

static class_set_item_t* class_bracketed_node_add_literal(node_class_bracketed_t* node,
                                                          char value) {
   if (node->num_items == node->items_size) {
      node->items_size = node->items_size == 0 ? 1 : node->items_size * 2;
      node->items =
          (class_set_item_t**)xrealloc(node->items, node->items_size * sizeof(class_set_item_t*));
   }
   class_set_item_t* item = &node->items[node->num_items++];
   item->kind = CLASS_SET_ITEM_KIND_LITERAL;
   item->literal = value;
   return item;
}

static class_set_item_t* class_bracketed_node_add_range(node_class_bracketed_t* node, char start,
                                                        char end) {
   if (node->num_items == node->items_size) {
      node->items_size = node->items_size == 0 ? 1 : node->items_size * 2;
      node->items =
          (class_set_item_t**)xrealloc(node->items, node->items_size * sizeof(class_set_item_t*));
   }
   class_set_item_t* item = &node->items[node->num_items++];
   item->kind = CLASS_SET_ITEM_KIND_RANGE;
   item->range.start = start;
   item->range.end = end;
   return item;
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

// Whether the character is in the first set of 'factor'
static int in_factor_first_set(char c) {
   return (is_valid_character(c) == true && is_special_character(c) == false) || c == '(' ||
          c == '\\' || c == '.' || c == '[';
}
