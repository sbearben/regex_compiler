#ifndef PARSE_H
#define PARSE_H

#define LITERAL_START 32
#define LITERAL_END 126
#define NUM_LITERALS (LITERAL_END - LITERAL_START + 1)
#define ASCII_SIZE 128

typedef struct ast_node ast_node_t;
typedef struct ast_node_option ast_node_option_t;
typedef struct ast_node_concat ast_node_concat_t;
typedef struct ast_node_repitition ast_node_repitition_t;
typedef struct ast_node_dot ast_node_dot_t;
typedef struct ast_node_literal ast_node_literal_t;
typedef struct ast_node_class_bracketed ast_node_class_bracketed_t;

typedef struct class_set_item class_set_item_t;
typedef struct class_set_range class_set_range_t;

typedef enum {
   NODE_KIND_OPTION,
   NODE_KIND_CONCAT,
   NODE_KIND_REPITITION,
   NODE_KIND_DOT,
   NODE_KIND_LITERAL,
   NODE_KIND_CLASS_BRACKETED,
} NodeKind;

typedef enum {
   REPITITION_KIND_ZERO_OR_ONE,
   REPITITION_KIND_ZERO_OR_MORE,
   REPITITION_KIND_ONE_OR_MORE,
} RepetitionKind;

typedef enum {
   CLASS_SET_ITEM_KIND_LITERAL,
   CLASS_SET_ITEM_KIND_RANGE,
} ClassSetItemKind;

struct ast_node {
      NodeKind kind;
      union {
            ast_node_option_t* option;
            ast_node_concat_t* concat;
            ast_node_repitition_t* repitition;
            ast_node_dot_t* dot;
            ast_node_literal_t* literal;
            ast_node_class_bracketed_t* class_bracketed;
      };
};

struct ast_node_option {
      // Could be a list of child nodes
      ast_node_t* left;
      ast_node_t* right;
};

struct ast_node_concat {
      ast_node_t* left;
      ast_node_t* right;
};

struct ast_node_repitition {
      RepetitionKind kind;
      ast_node_t* child;
};

struct ast_node_dot {
      char nothing;
};

struct ast_node_literal {
      char value;
};

struct ast_node_class_bracketed {
      int negated;
      int num_items;       // number of items in the set
      int items_capacity;  // capacity of the items array (>= num_items)
      class_set_item_t* items;
};

struct class_set_range {
      char start;
      char end;
};

struct class_set_item {
      ClassSetItemKind kind;
      union {
            char literal;
            class_set_range_t range;
      };
};

/**
 * Parses a regex pattern into an AST.
 */
ast_node_t* parse_regex(char* pattern);

/**
 * Frees the AST.
 */
void free_ast(ast_node_t*);

/**
 * Returns if a character is a valid chracter in the regex language.
*/
int is_valid_character(char);

#endif  // PARSE_H
