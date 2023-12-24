#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct list_node {
      void* data;
      struct list_node* next;
} list_node_t;

typedef struct list {
      list_node_t* head;
      list_node_t* tail;
      void (*destructor)(list_node_t*);
} list_t;

void list_initialize(list_t* list, void (*destructor)(list_node_t*));
void list_push(list_t* list, void* data);
list_t* list_concat(list_t* list1, list_t* list2);
bool list_contains(list_t* list, void* data, int (*compare)(void*, void*));
void list_release(list_t* list);
void list_default_destructor(list_node_t*);
void list_for_each(list_t* list, void (*execute)(list_node_t*));

void list_initialize(list_t* list, void (*destructor)(list_node_t*)) {
   list->head = NULL;
   list->tail = NULL;
   list->destructor = destructor;
}

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

bool list_contains(list_t* list, void* data, int (*compare)(void*, void*)) {
   list_node_t* node = list->head;
   bool contains = false;
   while (node != NULL && contains == false) {
      contains = compare != NULL ? compare(node->data, data) == 0 : node->data == data;
      node = node->next;
   }
   return contains;
}

// Deallocate the list using the defined destructor (or free if not defined)
void list_release(list_t* list) {
   if (!list) {
      return;
   }
   list_for_each(list, list->destructor ? list->destructor : list_default_destructor);
   free(list);
}

void list_default_destructor(list_node_t* node) {
   free(node->data);
   free(node);
}

// Traverse each node in the list
void list_for_each(list_t* list, void (*execute)(list_node_t*)) {
   list_node_t* current = list->head;
   list_node_t* next;

   while (current != NULL) {
      next = current->next;
      execute(current);
      current = next;
   }
}

// Traverse each element in the list
