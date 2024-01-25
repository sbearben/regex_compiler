#include "nfa.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

typedef enum CharacterClass { CHARACTER_CLASS_ANY } CharClass_t;

// Public API

// Constructors per parser grammar
static nfa_t* new_choice_nfa(nfa_t*, nfa_t*);      // 'a|b'
static nfa_t* new_concat_nfa(nfa_t*, nfa_t*);      // 'ab'
static nfa_t* new_repetition_nfa(nfa_t*);          // 'a*'
static nfa_t* new_min_one_repetition_nfa(nfa_t*);  // 'a+'
static nfa_t* new_optional_nfa(nfa_t*);            // 'a?'
static nfa_t* new_literal_nfa(char);               // 'a'
// Chracter classes
static nfa_t* new_character_class_nfa(CharClass_t);
static nfa_t* new_class_bracketed_nfa(ast_node_class_bracketed_t*);
static nfa_t* nfa_from_character_set(char*);

// Helpers for the NFA constructors
static nfa_t* new_nfa();
static void nfa_consume_nodes(nfa_t*, nfa_t*);
static void nfa_set_start_end(nfa_t*, nfa_node_t*, nfa_node_t*);
static nfa_node_t* nfa_new_node(nfa_t*, int);
static nfa_node_t* new_node(int);
static nfa_edge_t* new_edges(int);
static void node_set_edges(nfa_node_t*, nfa_edge_t*, int);
static void init_epsilon(nfa_edge_t*);
// Traversal
typedef void (*on_node_f)(nfa_node_t*);
static void nfa_traverse(nfa_t*, on_node_f);
static void nodes_traverse(nfa_node_t*, on_node_f, list_t*);
static void free_nfa_list_node(void*);
static void log_node(nfa_node_t*);
// Generic helpers
static void* xmalloc(size_t);
static void error(char*);

static int node_id = 0;

/**
 * Public API
*/

nfa_t* nfa_from_ast(ast_node_t* root) {
   nfa_t* nfa;

   switch (root->kind) {
      case NODE_KIND_OPTION: {
         nfa_t* left = nfa_from_ast(root->option->left);
         nfa_t* right = nfa_from_ast(root->option->right);
         nfa = new_choice_nfa(left, right);
         break;
      }
      case NODE_KIND_CONCAT: {
         nfa_t* left = nfa_from_ast(root->concat->left);
         nfa_t* right = nfa_from_ast(root->concat->right);
         nfa = new_concat_nfa(left, right);
         break;
      }
      case NODE_KIND_REPITITION: {
         nfa_t* child = nfa_from_ast(root->repitition->child);
         switch (root->repitition->kind) {
            case REPITITION_KIND_ZERO_OR_MORE:
               nfa = new_repetition_nfa(child);
               break;
            case REPITITION_KIND_ZERO_OR_ONE:
               nfa = new_optional_nfa(child);
               break;
            case REPITITION_KIND_ONE_OR_MORE:
               nfa = new_min_one_repetition_nfa(child);
               break;
            default:
               error("[nfa_from_ast] unexpected repitition kind");
         }
         break;
      }
      case NODE_KIND_DOT: {
         nfa = new_character_class_nfa(CHARACTER_CLASS_ANY);
         break;
      }
      case NODE_KIND_LITERAL: {
         nfa = new_literal_nfa(root->literal->value);
         break;
      }
      case NODE_KIND_CLASS_BRACKETED: {
         nfa = new_class_bracketed_nfa(root->class_bracketed);
         break;
      }
   }
   return nfa;
}

int nfa_num_states(nfa_t* nfa) { return list_size(nfa->__nodes); }

char* nfa_language(nfa_t* nfa) {
   if (nfa->__language != NULL) {
      return nfa->__language;
   }

   static char seen_characters[128];
   memset(seen_characters, 0, sizeof seen_characters);

   nfa->__language = (char*)malloc(128);
   int lang_index = 0;
   char edge_value = '\0';

   list_node_t* current;
   list_traverse(nfa->__nodes, current) {
      nfa_node_t* nfa_node = (nfa_node_t*)current->data;
      for (int i = 0; i < nfa_node->num_edges; i++) {
         if (nfa_node->edges[i].is_epsilon) {
            continue;
         }

         edge_value = nfa_node->edges[i].value;
         if (seen_characters[(int)edge_value] == 0) {
            nfa->__language[lang_index++] = edge_value;
            seen_characters[(int)edge_value] = 1;
         }
      }
   }
   nfa->__language[lang_index] = '\0';

   return nfa->__language;
}

nfa_node_t* nfa_node_find_transition(nfa_node_t* nfa_node, char ch) {
   for (int i = 0; i < nfa_node->num_edges; i++) {
      if (nfa_node->edges[i].value == ch) {
         return nfa_node->edges[i].to;
      }
   }
   return NULL;
}

void free_nfa(nfa_t* nfa) {
   // Releaase all nodes in the nfa
   list_release(nfa->__nodes);
   if (nfa->__language != NULL) {
      free(nfa->__language);
   }
   free(nfa);
}

void log_nfa(nfa_t* nfa) {
   printf("NFA (start - %d):\n", nfa->start->id);
   nfa_traverse(nfa, log_node);
}

/**
 * NFA Constructors (private)
*/

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

static nfa_t* new_character_class_nfa(CharClass_t cctype) {
   // '.' matches any single character except line terminators \n, \r (but includes \t)
   static char any_characters[NUM_LITERALS + 2] = {0};

   switch (cctype) {
      case CHARACTER_CLASS_ANY:
         if (any_characters[0] == 0) {
            int index_offset = 0;
            for (char cl = LITERAL_START; cl <= LITERAL_END; cl++) {
               any_characters[index_offset++] = cl;
            }
            any_characters[index_offset++] = '\t';
            any_characters[index_offset] = '\0';
         }
         return nfa_from_character_set(any_characters);
      default:
         error("[new_character_class] unexpected character class");
   }
   return NULL;
}

static nfa_t* new_class_bracketed_nfa(ast_node_class_bracketed_t* node) {
   static char seen_characters[ASCII_SIZE];
   memset(seen_characters, 0, sizeof seen_characters);
   int max_character_count = 0;

   for (int i = 0; i < node->num_items; i++) {
      switch (node->items[i].kind) {
         case CLASS_SET_ITEM_KIND_LITERAL:
            seen_characters[(int)node->items[i].literal] = 1;
            max_character_count++;
            break;
         case CLASS_SET_ITEM_KIND_RANGE:
            for (char ch = node->items[i].range.start; ch <= node->items[i].range.end; ch++) {
               seen_characters[(int)ch] = 1;
               max_character_count++;
            }
            break;
      }
   }

   if (node->negated == 1) {
      max_character_count = 0;
      for (int i = 0; i < ASCII_SIZE; i++) {
         if (is_valid_character((char)i) == true) {
            seen_characters[i] = seen_characters[i] == 0 ? 1 : 0;
            if (seen_characters[i] == 1) {
               max_character_count++;
            }
         }
      }
   }

   // Create nfa from seen characters
   char* characters = (char*)xmalloc((MIN(max_character_count, ASCII_SIZE) + 1) * sizeof(char));
   int ch_index = 0;
   for (int i = 0; i < ASCII_SIZE; i++) {
      if (seen_characters[i] == 1) {
         characters[ch_index++] = i;
      }
   }
   characters[ch_index] = '\0';

   nfa_t* temp = nfa_from_character_set(characters);
   free(characters);

   return temp;
}

static nfa_t* nfa_from_character_set(char* characters) {
   nfa_t* nfa = new_nfa();
   nfa_node_t* start_node = nfa_new_node(nfa, strlen(characters));
   nfa_node_t* end_node = nfa_new_node(nfa, 0);
   nfa_set_start_end(nfa, start_node, end_node);

   for (int i = 0; i < strlen(characters); i++) {
      nfa->start->edges[i].value = characters[i];
      nfa->start->edges[i].is_epsilon = false;
      nfa->start->edges[i].to = nfa->end;
   }
   return nfa;
}

/**
 * Helprs for the NFA constructors (private)
*/

static nfa_t* new_nfa() {
   nfa_t* nfa = (nfa_t*)xmalloc(sizeof(nfa_t));
   nfa->start = NULL;
   nfa->end = NULL;
   nfa->__language = NULL;

   nfa->__nodes = (list_t*)malloc(sizeof(list_t));
   list_initialize(nfa->__nodes, free_nfa_list_node);

   return nfa;
}

// Adds nodes from the 'other' nfa to target nfa and frees the 'other' nodes list (but not its elements whuch have been transfered)
static void nfa_consume_nodes(nfa_t* nfa, nfa_t* other) {
   list_concat(nfa->__nodes, other->__nodes);
   free(other->__nodes);
}

// Set start and end node, and set end node to be accepting
static void nfa_set_start_end(nfa_t* nfa, nfa_node_t* start, nfa_node_t* end) {
   nfa->start = start;
   nfa->end = end;
   nfa->end->is_accepting = true;
}

static nfa_node_t* nfa_new_node(nfa_t* nfa, int num_edges) {
   nfa_node_t* node = new_node(num_edges);
   list_push(nfa->__nodes, node);

   return node;
}

// Alloc a node and its edges -> edges are empty and need to be initialized
static nfa_node_t* new_node(int num_edges) {
   nfa_node_t* node = (nfa_node_t*)xmalloc(sizeof(nfa_node_t));
   node->id = node_id++;
   node->is_accepting = false;

   if (num_edges > 0) {
      nfa_edge_t* edges = new_edges(num_edges);
      node->edges = edges;
      node->num_edges = num_edges;
   } else {
      node->edges = NULL;
      node->num_edges = 0;
   }

   return node;
}

static nfa_edge_t* new_edges(int num) {
   nfa_edge_t* edges = (nfa_edge_t*)xmalloc(num * sizeof(nfa_edge_t));
   for (int i = 0; i < num; i++) {
      edges[i].to = NULL;
   }

   return edges;
}

static void node_set_edges(nfa_node_t* node, nfa_edge_t* edges, int num_edges) {
   // I believe it should always be the case that any node we are setting edges on didn't
   // have any edges previously - thus there is no need to free any previous edges.
   assert(node->edges == NULL);
   node->edges = edges;
   node->num_edges = num_edges;
}

static void init_epsilon(nfa_edge_t* edge) {
   edge->value = '\0';
   edge->is_epsilon = true;
   edge->to = NULL;
}

static void nfa_traverse(nfa_t* nfa, on_node_f on_node) {
   list_t* seen_nodes = (list_t*)malloc(sizeof(list_t));
   list_initialize(seen_nodes, list_noop_data_destructor);

   nodes_traverse(nfa->start, on_node, seen_nodes);

   list_release(seen_nodes);
}

static void nodes_traverse(nfa_node_t* node, on_node_f on_node, list_t* seen_nodes) {
   if (list_contains(seen_nodes, node, NULL)) {
      return;
   }
   list_push(seen_nodes, node);
   if (on_node != NULL) {
      on_node(node);
   }

   for (int i = 0; i < node->num_edges; i++) {
      nodes_traverse(node->edges[i].to, on_node, seen_nodes);
   }
}

static void free_nfa_list_node(void* data) {
   nfa_node_t* node = (nfa_node_t*)data;
   // printf("freeing node %d\n", node->id);
   if (node->edges != NULL) {
      free(node->edges);
   }
   // printf("freed node\n");
   free(node);
}

static void log_node(nfa_node_t* node) {
   printf("Node %d - num_edges: %d, %s\n", node->id, node->num_edges,
          node->is_accepting ? "accepting" : "not accepting");
   for (int i = 0; i < node->num_edges; i++) {
      printf("    ");
      nfa_edge_t* edge = &node->edges[i];
      int to_id = edge->to != NULL ? edge->to->id : -1;
      if (edge->is_epsilon) {
         printf("Edge: epsilon, to: %d\n", to_id);
      } else {
         printf("Edge: %c, to: %d\n", edge->value, to_id);
      }
   }
}

/**
 * Generic helpers
*/

static void* xmalloc(size_t size) {
   void* ptr = malloc(size);
   if (!ptr) {
      fprintf(stderr, "malloc failed\n");
      exit(EXIT_FAILURE);
   }
   return ptr;
}

static void error(char* msg) {
   fprintf(stderr, "Error: %s\n", msg);
   exit(EXIT_FAILURE);
}
