#ifndef PARSE_H
#define PARSE_H

ast_node_t* parse_regex(char* pattern);

typedef struct node_option node_option_t;
typedef struct node_concat node_concat_t;
typedef struct node_repitition node_repitition_t;
typedef struct node_dot node_dot_t;
typedef struct node_literal node_literal_t;
typedef struct node_class_bracketed node_class_bracketed_t;

typedef struct class_set_item class_set_item_t;
typedef struct class_set_range class_set_range_t;

typedef enum {
   NODE_KIND_OPTION,
   NODE_KIND_CONCAT,
   NODE_KIND_REPITITION,
   NODE_KIND_LITERAL,
   NODE_KIND_DOT,
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

typedef struct regex_parse_tree {
      ast_node_t* root;
} regex_parse_tree_t;

typedef struct ast_node {
      NodeKind kind;
      union {
            node_option_t option;
            node_concat_t concat;
            node_repitition_t repitition;
            node_dot_t dot;
            node_literal_t literal;
            node_class_bracketed_t class_bracketed;
      };
} ast_node_t;

struct node_option {
      // Could be a list of child nodes
      ast_node_t* left;
      ast_node_t* right;
};

struct node_concat {
      ast_node_t* left;
      ast_node_t* right;
};

struct node_repitition {
      RepetitionKind rep_kind;
      ast_node_t* child;
};

struct node_dot {
      char nothing;
};

struct node_literal {
      char value;
};

struct node_class_bracketed {
      int negated;
      int num_items;
      int items_size;
      class_set_item_t* items;
};

struct class_set_item {
      ClassSetItemKind kind;
      union {
            char literal;
            class_set_range_t range;
      };
};

struct class_set_range {
      char start;
      char end;
};

#endif  // PARSE_H
