#include "list.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

static int default_comparator(void* data1, void* data2);

void list_initialize(list_t* list, void (*destructor)(list_node_t*)) {
   list->head = NULL;
   list->tail = NULL;
   list->destructor = destructor;
}

int list_size(list_t* list) {
   int size = 0;
   list_node_t* node;
   list_traverse(list, node) { size++; }

   return size;
}

bool list_empty(list_t* list) { return list->head == NULL; }

void list_push(list_t* list, void* data) {
   list_node_t* node = (list_node_t*)malloc(sizeof(list_node_t));
   node->data = data;
   node->next = NULL;

   if (list->head == NULL) {
      list->head = node;
   } else {
      list->tail->next = node;
   }
   list->tail = node;
}

void* list_deque(list_t* list) {
   list_node_t* node = list->head;
   void* data = node->data;

   if (node == NULL) {
      return NULL;
   }

   list->head = node->next;
   free(node);

   return data;
}

// void list_add_head(list_t* list, void* data) {
//    list_node_t* node = (list_node_t*)malloc(sizeof(list_node_t));
//    node->data = data;

//    if (list->head == NULL) {
//       list->tail = node;
//       node->next = NULL;
//    } else {
//       node->next = list->head;
//    }
//    list->head = node;
// }

// Concat list2 onto list1 (TODO: maybe should copy list2 first)
list_t* list_concat(list_t* list1, list_t* list2) {
   if (list1->head == NULL) {
      assert(list1->tail == NULL);
      list1->head = list2->head;
      list1->tail = list2->tail;
   } else {
      list1->tail->next = list2->head;
      list1->tail = list2->tail;
   }
   return list1;
}

void* list_find(list_t* list, void* data, int (*compare)(void*, void*)) {
   int (*compare_func)(void*, void*) = compare != NULL ? compare : default_comparator;

   list_node_t* node;
   list_traverse(list, node) {
      if (compare_func(node->data, data) == 0) {
         return node->data;
      }
   }

   return NULL;
}

bool list_contains(list_t* list, void* data, int (*compare)(void*, void*)) {
   return list_find(list, data, compare) != NULL ? true : false;
}

// Sort the list using the given compare function
void list_sort(list_t* list, int (*compare)(void*, void*)) {
   int (*compare_func)(void*, void*) = compare != NULL ? compare : default_comparator;
   list_node_t* current = list->head;
   list_node_t* next;
   list_node_t* prev = NULL;

   while (current != NULL) {
      next = current->next;
      while (next != NULL && compare_func(current->data, next->data) > 0) {
         prev = next;
         next = next->next;
      }
      if (prev != NULL) {
         prev->next = current;
      } else {
         list->head = current;
      }
      current->next = next;
      prev = current;
      current = next;
   }
}

// Deallocate the list using the defined destructor (or free if not defined)
void list_release(list_t* list) {
   if (!list) {
      return;
   }

   void (*destructor)(list_node_t*) = list->destructor ? list->destructor : list_default_destructor;
   list_node_t* current = list->head;
   list_node_t* next;

   while (current != NULL) {
      next = current->next;
      destructor(current);
      current = next;
   }

   free(list);
}

// Default destructor for list nodes (free the data and the node)
void list_default_destructor(list_node_t* node) {
   free(node->data);
   free(node);
}

// Destructor for list nodes that frees the node only but not the data
void list_noop_data_destructor(list_node_t* node) { free(node); }

static int default_comparator(void* data1, void* data2) {
   if (data1 == data2) return 0;
   if (data1 > data2) return 1;
   return -1;
}
