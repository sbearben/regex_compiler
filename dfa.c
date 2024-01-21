#include "dfa.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

struct dfa {
      dfa_node_t* start;
      list_t* __nodes;
};

struct dfa_node {
      char* id;
      bool is_accepting;
      list_t* edges;
};

struct dfa_edge {
      char value;
      dfa_node_t* to;
};

/**
 * Note: Any list that holds a copy of a pointer uses list_noop_data_destructor as the destructor.
 * In terms of nfa_nodes, the nfa is the owner of all nodes (an eclosure never does).
 */
typedef struct epsilon_closure {
      char* id;
      list_t* nodes;  // list of nfa_node_t
} epsilon_closure_t;

static epsilon_closure_t* new_epsilon_closure();
static epsilon_closure_t* compute_epsilon_closure(nfa_node_t*);
static epsilon_closure_t* compute_epsilon_closure_for_set(list_t*, char*);
static void __compute_epsilon_closure(nfa_node_t*, epsilon_closure_t*);

static dfa_node_t* dfa_node_from_epsilon_closure(epsilon_closure_t*);
static dfa_node_t* dfa_find_node(dfa_t*, char*);
static void dfa_node_add_edge(dfa_node_t*, char, dfa_node_t*);
static char* create_id_for_set(list_t*);
static list_t* compute_transition_symbols(epsilon_closure_t*);
static list_t* compute_move_set(list_t*, char);

static void free_dfa_list_node(void*);
static void free_epsilon_closure(epsilon_closure_t*);

static int nfa_node_comparator(void*, void*);
static int dfa_find_by_id_comparator(void*, void*);

bool dfa_accepts(dfa_t* dfa, char* str, int len) {
   dfa_node_t* current = dfa->start;

   for (int i = 0; i < len; i++) {
      bool found_transition = false;
      list_node_t* current_edge;
      list_traverse(current->edges, current_edge) {
         dfa_edge_t* edge = (dfa_edge_t*)current_edge->data;
         if (edge->value == str[i]) {
            found_transition = true;
            current = edge->to;
            break;
         }
      }
      if (!found_transition) {
         return false;
      }
   }

   return current->is_accepting;
}

dfa_t* dfa_from_nfa(nfa_t* nfa) {
   // Create dfa
   dfa_t* dfa = (dfa_t*)xmalloc(sizeof(dfa_t));
   dfa->start = NULL;
   dfa->__nodes = (list_t*)malloc(sizeof(list_t));
   list_initialize(dfa->__nodes, free_dfa_list_node);

   // Create initial eclosure from starting node of nfa, then create dfa_node from the eclosure
   epsilon_closure_t* initial_closure = compute_epsilon_closure(nfa->start);

   // Create a stack of eclosures to process - this stack is empty by end of while loop
   list_t* eclosures_stack = (list_t*)malloc(sizeof(list_t));
   list_initialize(eclosures_stack, NULL);
   list_push(eclosures_stack, initial_closure);

   // Create initial dfa_node from initial eclosure and add to dfa
   dfa_node_t* initial_dfa_node = dfa_node_from_epsilon_closure(initial_closure);
   list_push(dfa->__nodes, initial_dfa_node);
   dfa->start = initial_dfa_node;

   dfa_node_t* current_dfa_node;
   epsilon_closure_t* current_closure;
   while (!list_empty(eclosures_stack)) {
      // Have to free current_closure since it's being removed from list
      current_closure = (epsilon_closure_t*)list_deque(eclosures_stack);
      current_dfa_node = dfa_find_node(dfa, current_closure->id);
      assert(current_dfa_node != NULL);

      list_t* transitions = compute_transition_symbols(current_closure);

      list_node_t* current_symbol_node;
      list_traverse(transitions, current_symbol_node) {
         char transition_symbol = (char)current_symbol_node->data;

         // Get/create dfa_node and add edge from current_dfa_node to next_dfa_node
         list_t* move_result = compute_move_set(current_closure->nodes, transition_symbol);
         char* next_dfa_node_id = create_id_for_set(move_result);
         dfa_node_t* next_dfa_node = dfa_find_node(dfa, next_dfa_node_id);

         if (next_dfa_node == NULL) {
            epsilon_closure_t* next_closure =
                compute_epsilon_closure_for_set(move_result, next_dfa_node_id);
            list_push(eclosures_stack, next_closure);

            next_dfa_node = dfa_node_from_epsilon_closure(next_closure);
            list_push(dfa->__nodes, next_dfa_node);
         }
         dfa_node_add_edge(current_dfa_node, transition_symbol, next_dfa_node);

         free(next_dfa_node_id);
         list_release(move_result);
      };
      free_epsilon_closure(current_closure);
      list_release(transitions);
   }
   list_release(eclosures_stack);

   return dfa;
}

void log_dfa(dfa_t* dfa) {
   printf("DFA (start - %s):\n", dfa->start->id);

   list_node_t* current;
   list_traverse(dfa->__nodes, current) {
      dfa_node_t* node = (dfa_node_t*)current->data;
      printf("Node %s - num_edges: %d, %s\n", node->id, list_size(node->edges),
             node->is_accepting ? "accepting" : "not accepting");

      list_node_t* current_edge;
      list_traverse(node->edges, current_edge) {
         dfa_edge_t* edge = (dfa_edge_t*)current_edge->data;
         printf("    Edge: %c -> %s\n", edge->value, edge->to->id);
      }
   }
}

static epsilon_closure_t* new_epsilon_closure() {
   epsilon_closure_t* epsilon_closure = (epsilon_closure_t*)xmalloc(sizeof(epsilon_closure_t));
   epsilon_closure->id = NULL;

   epsilon_closure->nodes = (list_t*)malloc(sizeof(list_t));
   list_initialize(epsilon_closure->nodes, list_noop_data_destructor);

   return epsilon_closure;
}

static epsilon_closure_t* compute_epsilon_closure(nfa_node_t* nfa_node) {
   epsilon_closure_t* epsilon_closure = new_epsilon_closure();
   epsilon_closure->id = (char*)xmalloc(sizeof(char) * num_places(nfa_node->id) + 1);
   sprintf(epsilon_closure->id, "%d", nfa_node->id);

   __compute_epsilon_closure(nfa_node, epsilon_closure);

   return epsilon_closure;
}

static epsilon_closure_t* compute_epsilon_closure_for_set(list_t* nfa_nodes, char* id) {
   // Implementation would be a lot nicer with a proper set data structure
   epsilon_closure_t* set_eclosure = new_epsilon_closure();
   set_eclosure->id = (char*)xmalloc(sizeof(char) * strlen(id) + 1);
   strcpy(set_eclosure->id, id);

   list_node_t* current_nfa;
   list_traverse(nfa_nodes, current_nfa) {
      epsilon_closure_t* node_closure = compute_epsilon_closure((nfa_node_t*)current_nfa->data);

      list_node_t* current_closure_nfa_node;
      list_traverse(node_closure->nodes, current_closure_nfa_node) {
         if (!list_contains(set_eclosure->nodes, current_closure_nfa_node->data, NULL)) {
            list_push(set_eclosure->nodes, current_closure_nfa_node->data);
         }
      }
      free_epsilon_closure(node_closure);
   }

   return set_eclosure;
}

static void __compute_epsilon_closure(nfa_node_t* nfa_node, epsilon_closure_t* epsilon_closure) {
   if (list_contains(epsilon_closure->nodes, nfa_node, NULL)) {
      return;
   }

   list_push(epsilon_closure->nodes, nfa_node);

   for (int i = 0; i < nfa_node->num_edges; i++) {
      if (nfa_node->edges[i].is_epsilon) {
         __compute_epsilon_closure(nfa_node->edges[i].to, epsilon_closure);
      }
   }
}

dfa_node_t* dfa_node_from_epsilon_closure(epsilon_closure_t* epsilon_closure) {
   // Create dfa_node
   dfa_node_t* dfa_node = (dfa_node_t*)xmalloc(sizeof(dfa_node_t));
   dfa_node->id = (char*)xmalloc(sizeof(char) * strlen(epsilon_closure->id) + 1);
   strcpy(dfa_node->id, epsilon_closure->id);
   dfa_node->is_accepting = false;
   dfa_node->edges = (list_t*)malloc(sizeof(list_t));
   list_initialize(dfa_node->edges, NULL);

   // Determine if dfa_node is accepting
   list_node_t* current;
   list_traverse(epsilon_closure->nodes, current) {
      if (((nfa_node_t*)current->data)->is_accepting) {
         dfa_node->is_accepting = true;
         break;
      }
   }

   return dfa_node;
}

static dfa_node_t* dfa_find_node(dfa_t* dfa, char* id) {
   return list_find(dfa->__nodes, id, dfa_find_by_id_comparator);
}

static void dfa_node_add_edge(dfa_node_t* dfa_node, char symbol, dfa_node_t* to) {
   dfa_edge_t* edge = (dfa_edge_t*)xmalloc(sizeof(dfa_edge_t));
   edge->value = symbol;
   edge->to = to;

   list_push(dfa_node->edges, edge);
}

static char* create_id_for_set(list_t* nfa_nodes) {
   list_sort(nfa_nodes, nfa_node_comparator);
   int id_length = 0;

   // Calculate length of id
   list_node_t* current;
   list_traverse(nfa_nodes, current) {
      id_length += num_places(((nfa_node_t*)current->data)->id) + 1;
   }

   // Don't need +1 for null terminator since length calculation always adds 1 extra
   char* id = (char*)malloc(sizeof(char) * id_length);
   id[0] = '\0';

   // Create id
   list_traverse(nfa_nodes, current) {
      if (id[0] == '\0') {
         sprintf(id, "%d", ((nfa_node_t*)current->data)->id);
      } else {
         sprintf(id, "%s/%d", id, ((nfa_node_t*)current->data)->id);
      }
   }

   return id;
}

static list_t* compute_transition_symbols(epsilon_closure_t* closure) {
   list_t* symbols = (list_t*)malloc(sizeof(list_t));
   list_initialize(symbols, list_noop_data_destructor);

   list_node_t* current;
   list_traverse(closure->nodes, current) {
      nfa_node_t* node = (nfa_node_t*)current->data;
      for (int i = 0; i < node->num_edges; i++) {
         if (!node->edges[i].is_epsilon &&
             !list_contains(symbols, (void*)node->edges[i].value, NULL)) {
            list_push(symbols, (void*)node->edges[i].value);
         }
      }
   }

   return symbols;
}

static list_t* compute_move_set(list_t* nfa_nodes, char symbol) {
   list_t* nfa_nodes_with_transition = (list_t*)malloc(sizeof(list_t));
   list_initialize(nfa_nodes_with_transition, list_noop_data_destructor);

   list_node_t* current;
   list_traverse(nfa_nodes, current) {
      nfa_node_t* nfa_node = (nfa_node_t*)current->data;

      for (int i = 0; i < nfa_node->num_edges; i++) {
         if (nfa_node->edges[i].value == symbol) {
            list_push(nfa_nodes_with_transition, nfa_node->edges[i].to);
         }
      }
   }

   return nfa_nodes_with_transition;
}

/**
 * Destructors
 */

void free_dfa(dfa_t* dfa) {
   list_release(dfa->__nodes);
   free(dfa);
}

static void free_dfa_list_node(void* data) {
   dfa_node_t* node = (dfa_node_t*)data;
   free(node->id);
   list_release(node->edges);
   free(node);
}

static void free_epsilon_closure(epsilon_closure_t* epsilon_closure) {
   free(epsilon_closure->id);
   list_release(epsilon_closure->nodes);
   free(epsilon_closure);
}

/**
 * Comparators
 */

static int nfa_node_comparator(void* data1, void* data2) {
   return ((nfa_node_t*)data1)->id - ((nfa_node_t*)data2)->id;
}

static int dfa_find_by_id_comparator(void* data1, void* data2) {
   return strcmp(((dfa_node_t*)data1)->id, (char*)data2);
}
