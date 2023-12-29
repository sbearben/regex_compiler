#include "dfa.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"
#include "nfa.h"
#include "utils.h"

// TODO: can we get rid of this intermediate struct?
typedef struct epsilon_closure {
      char* id;
      list_t* nodes;  // list of nfa_node_t
      bool marked;
} epsilon_closure_t;

static epsilon_closure_t* new_epsilon_closure();
static epsilon_closure_t* compute_epsilon_closure(node_t* node);
static epsilon_closure_t* compute_epsilon_closure_for_set(list_t* nfa_nodes);
static void __compute_epsilon_closure(node_t* node, epsilon_closure_t* epsilon_closure);
static void free_epsilon_closure(epsilon_closure_t* epsilon_closure);

static dfa_node_t* dfa_node_from_epsilon_closure(epsilon_closure_t* epsilon_closure);
static void dfa_node_add_edge(dfa_node_t* dfa_node, char symbol, dfa_node_t* to);
static char* create_id_for_set(list_t* nfa_nodes);
static list_t* compute_transition_symbols(epsilon_closure_t* closure);
static list_t* compute_move_set(list_t* nodes, char symbol);
static epsilon_closure_t* find_unmarked_closure(list_t* eclosures);

static int nfa_node_comparator(void* data1, void* data2);
static int epsilon_closure_comparator(void* data1, void* data2);
static int dfa_find_by_id_comparator(void* data1, void* data2);

dfa_t* dfa_from_nfa(nfa_t* nfa) {
   // Create dfa
   dfa_t* dfa = (dfa_t*)xmalloc(sizeof(dfa_t));
   dfa->start = NULL;
   dfa->__nodes = (list_t*)malloc(sizeof(list_t));
   // TODO: add destructor
   list_initialize(dfa->__nodes, NULL);

   // Create initial eclosure from starting node of nfa, then create dfa_node from the eclosure
   epsilon_closure_t* initial_closure = compute_epsilon_closure(nfa->start);

   // Create a stack of eclosures to process
   list_t* eclosures_stack = (list_t*)malloc(sizeof(list_t));
   // TODO: destructor
   list_initialize(eclosures_stack, NULL);
   list_push(eclosures_stack, initial_closure);

   // Create initial dfa_node from initial eclosure and add to dfa
   dfa_node_t* initial_dfa_node = dfa_node_from_epsilon_closure(initial_closure);
   list_push(dfa->__nodes, initial_dfa_node);
   dfa->start = initial_dfa_node;

   dfa_node_t* current_dfa_node;
   epsilon_closure_t* current_closure;
   while ((current_closure = find_unmarked_closure(eclosures_stack))) {
      current_closure->marked = true;
      current_dfa_node = list_find(dfa->__nodes, current_closure->id, dfa_find_by_id_comparator);

      list_t* transitions = compute_transition_symbols(current_closure);

      list_node_t* current_symbol_node;
      list_traverse(transitions, current_symbol_node) {
         char transition_symbol = (char)current_symbol_node->data;
         printf("[dfa_from_nfa] current_dfa_node %s - transition_symbol: %c\n",
                current_dfa_node->id, transition_symbol);

         list_t* move_result = compute_move_set(current_closure->nodes, transition_symbol);
         epsilon_closure_t* next_closure = compute_epsilon_closure_for_set(move_result);

         printf("[dfa_from_nfa] Next closure: %s\n\n", next_closure->id);

         if (!list_contains(eclosures_stack, next_closure, epsilon_closure_comparator)) {
            next_closure->marked = false;
            list_push(eclosures_stack, next_closure);
         }

         // Get/create dfa_node for next_closure and add edge from current_dfa_node to next_dfa_node
         dfa_node_t* next_dfa_node =
             list_find(dfa->__nodes, next_closure->id, dfa_find_by_id_comparator);
         if (next_dfa_node == NULL) {
            next_dfa_node = dfa_node_from_epsilon_closure(next_closure);
            list_push(dfa->__nodes, next_dfa_node);
         }
         dfa_node_add_edge(current_dfa_node, transition_symbol, next_dfa_node);

         list_release(move_result);
      };

      list_release(transitions);
   }

   list_release(eclosures_stack);

   return dfa;
}

void log_dfa(dfa_t* dfa) {
   printf("DFA:\n");
   printf("  Start: %s\n", dfa->start->id);

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
   epsilon_closure->marked = false;

   //  epsilon_closure->edges = NULL;
   //  epsilon_closure->num_edges = 0;

   return epsilon_closure;
}

static epsilon_closure_t* compute_epsilon_closure(node_t* node) {
   epsilon_closure_t* epsilon_closure = new_epsilon_closure();
   // Create string id from node id
   epsilon_closure->id = (char*)xmalloc(sizeof(char) * num_places(node->id) + 1);
   sprintf(epsilon_closure->id, "%d", node->id);

   __compute_epsilon_closure(node, epsilon_closure);

   return epsilon_closure;
}

// We can create e-closures for each node in the nfa
//   - id of this e-closure is the id of the node
// We can also create e-closures for sets of nodes in the nfa
//   - id of this e-closure would be the set of ids of the nfa nodes in the set
// These e-closures are what creates nodes in the dfa
//   - edges in dfa essentially connect e-closures

static epsilon_closure_t* compute_epsilon_closure_for_set(list_t* nfa_nodes) {
   epsilon_closure_t* set_eclosure = new_epsilon_closure();
   set_eclosure->id = create_id_for_set(nfa_nodes);

   // - compute epsilon closure for each node in the set
   // - create set union of all epsilon closures

   list_node_t* current_nfa;
   list_traverse(nfa_nodes, current_nfa) {
      epsilon_closure_t* node_closure = compute_epsilon_closure((node_t*)current_nfa->data);
      list_node_t* current_closure_nfa_node;
      list_traverse(node_closure->nodes, current_closure_nfa_node) {
         if (!list_contains(set_eclosure->nodes, current_closure_nfa_node->data, NULL)) {
            // TODO: could we make a `move(void**)` function that returns the value and sets the pointer to NULL?
            list_push(set_eclosure->nodes, current_closure_nfa_node->data);
            current_closure_nfa_node->data = NULL;
         }
      }
      // TODO: issue is `compute_epsilon_closure` always uses `list_noop_data_destructor` so the node data is never freed
      free_epsilon_closure(node_closure);
   }

   return set_eclosure;
}

static void __compute_epsilon_closure(node_t* node, epsilon_closure_t* epsilon_closure) {
   if (list_contains(epsilon_closure->nodes, node, NULL)) {
      return;
   }

   list_push(epsilon_closure->nodes, node);

   for (int i = 0; i < node->num_edges; i++) {
      if (node->edges[i].is_epsilon) {
         __compute_epsilon_closure(node->edges[i].to, epsilon_closure);
      }
   }
}

dfa_node_t* dfa_node_from_epsilon_closure(epsilon_closure_t* epsilon_closure) {
   dfa_node_t* dfa_node = (dfa_node_t*)xmalloc(sizeof(dfa_node_t));
   dfa_node->id = (char*)xmalloc(sizeof(char) * strlen(epsilon_closure->id) + 1);
   strcpy(dfa_node->id, epsilon_closure->id);
   dfa_node->is_accepting = false;

   dfa_node->edges = (list_t*)malloc(sizeof(list_t));
   // TODO: destructor
   list_initialize(dfa_node->edges, list_noop_data_destructor);

   list_node_t* current;
   list_traverse(epsilon_closure->nodes, current) {
      if (((node_t*)current->data)->is_accepting) {
         dfa_node->is_accepting = true;
         break;
      }
   }

   return dfa_node;
}

void dfa_node_add_edge(dfa_node_t* dfa_node, char symbol, dfa_node_t* to) {
   dfa_edge_t* edge = (dfa_edge_t*)xmalloc(sizeof(dfa_edge_t));
   edge->value = symbol;
   edge->to = to;

   list_push(dfa_node->edges, edge);
}

// Allocates a string that needs to be released
static char* create_id_for_set(list_t* nfa_nodes) {
   list_sort(nfa_nodes, nfa_node_comparator);

   char* id = (char*)malloc(sizeof(char) * 256);
   id[0] = '\0';

   list_node_t* current;
   list_traverse(nfa_nodes, current) {
      if (id[0] == '\0') {
         sprintf(id, "%d", ((node_t*)current->data)->id);
      } else {
         sprintf(id, "%s/%d", id, ((node_t*)current->data)->id);
      }
   }

   return id;
}

// Allocates a list_t* with nodes that need to be released
static list_t* compute_transition_symbols(epsilon_closure_t* closure) {
   list_t* symbols = (list_t*)malloc(sizeof(list_t));
   list_initialize(symbols, list_noop_data_destructor);

   list_node_t* current;
   list_traverse(closure->nodes, current) {
      node_t* node = (node_t*)current->data;
      for (int i = 0; i < node->num_edges; i++) {
         if (!node->edges[i].is_epsilon &&
             !list_contains(symbols, (void*)node->edges[i].value, NULL)) {
            list_push(symbols, (void*)node->edges[i].value);
         }
      }
   }

   return symbols;
}

// Allocates a list_t* with nodes that need to be released
static list_t* compute_move_set(list_t* nfa_nodes, char symbol) {
   list_t* nodes_with_transition = (list_t*)malloc(sizeof(list_t));
   list_initialize(nodes_with_transition, list_noop_data_destructor);

   list_node_t* current;
   list_traverse(nfa_nodes, current) {
      node_t* nfa_node = (node_t*)current->data;
      printf("[compute_move_set] Node %d - num_edges: %d\n", nfa_node->id, nfa_node->num_edges);
      for (int i = 0; i < nfa_node->num_edges; i++) {
         if (nfa_node->edges[i].value == symbol) {
            printf("[compute_move_set] Adding node %d - has transition on %c\n",
                   nfa_node->edges[i].to->id, symbol);
            list_push(nodes_with_transition, nfa_node->edges[i].to);
            // break;
         }
      }
   }

   return nodes_with_transition;
}

static epsilon_closure_t* find_unmarked_closure(list_t* eclosures) {
   list_node_t* current;
   list_traverse(eclosures, current) {
      epsilon_closure_t* ec = (epsilon_closure_t*)current->data;
      if (!ec->marked) {
         return ec;
      }
   }

   return NULL;
}

/**
 * Destructors
*/
static void free_epsilon_closure(epsilon_closure_t* epsilon_closure) {
   free(epsilon_closure->id);
   list_release(epsilon_closure->nodes);
   free(epsilon_closure);
}

/**
 * Comparators
*/

static int nfa_node_comparator(void* data1, void* data2) {
   return ((node_t*)data1)->id - ((node_t*)data2)->id;
}

static int epsilon_closure_comparator(void* data1, void* data2) {
   return strcmp(((epsilon_closure_t*)data1)->id, ((epsilon_closure_t*)data2)->id);
}

static int dfa_find_by_id_comparator(void* data1, void* data2) {
   return strcmp(((dfa_node_t*)data1)->id, (char*)data2);
}
