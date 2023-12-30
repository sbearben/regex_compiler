#include "nfa.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static int node_id = 0;

static void nodes_traverse(nfa_node_t* node, on_node_f, list_t* seen_nodes);
static void free_list_node(list_node_t* list_node);
static void log_node(nfa_node_t* node);

static void* xmalloc(size_t size) {
   void* ptr = malloc(size);
   if (!ptr) {
      fprintf(stderr, "malloc failed\n");
      exit(EXIT_FAILURE);
   }
   return ptr;
}

nfa_t* new_nfa() {
   nfa_t* nfa = (nfa_t*)xmalloc(sizeof(nfa_t));
   nfa->start = NULL;
   nfa->end = NULL;

   nfa->__nodes = (list_t*)malloc(sizeof(list_t));
   list_initialize(nfa->__nodes, free_list_node);

   return nfa;
}

// Adds nodes from the 'other' nfa to target nfa and frees the 'other' nodes list (but not its elements whuch have been transfered)
void nfa_consume_nodes(nfa_t* nfa, nfa_t* other) {
   list_concat(nfa->__nodes, other->__nodes);
   free(other->__nodes);
}

// Set start and end node, and set end node to be accepting
void nfa_set_start_end(nfa_t* nfa, nfa_node_t* start, nfa_node_t* end) {
   nfa->start = start;
   nfa->end = end;
   nfa->end->is_accepting = true;
}

nfa_node_t* nfa_new_node(nfa_t* nfa, int num_edges) {
   nfa_node_t* node = new_node(num_edges);
   list_push(nfa->__nodes, node);

   return node;
}

// Alloc a node and its edges -> edges are empty and need to be initialized
nfa_node_t* new_node(int num_edges) {
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

nfa_edge_t* new_edges(int num) {
   nfa_edge_t* edges = (nfa_edge_t*)xmalloc(num * sizeof(nfa_edge_t));
   for (int i = 0; i < num; i++) {
      edges[i].to = NULL;
   }

   return edges;
}

void node_set_edges(nfa_node_t* node, nfa_edge_t* edges, int num_edges) {
   // I believe it should always be the case that any node we are setting edges on didn't
   // have any edges previously - thus there is no need to free any previous edges.
   assert(node->edges == NULL);
   node->edges = edges;
   node->num_edges = num_edges;
}

void init_epsilon(nfa_edge_t* edge) {
   edge->value = '\0';
   edge->is_epsilon = true;
   edge->to = NULL;
}

void free_nfa(nfa_t* nfa) {
   // Releaase all nodes in the nfa
   list_release(nfa->__nodes);
   free(nfa);
}

void nfa_traverse(nfa_t* nfa, on_node_f on_node) {
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

static void free_list_node(list_node_t* list_node) {
   nfa_node_t* node = (nfa_node_t*)list_node->data;
   // printf("freeing node %d\n", node->id);
   if (node->edges != NULL) {
      free(node->edges);
   }
   // printf("freed node\n");
   free(node);
   free(list_node);
}

void log_nfa(nfa_t* nfa) {
   printf("NFA:\n");
   nfa_traverse(nfa, log_node);
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
