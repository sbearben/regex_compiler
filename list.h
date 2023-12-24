#include <stdbool.h>
#include <stdlib.h>

typedef struct list_node {
      void* data;
      struct list_node* next;
} list_node_t;

typedef struct list {
      list_node_t* head;
      list_node_t* tail;
      void (*destructor)(void*);
} list_t;

void list_initialize(list_t* list, void (*destructor)(void*));
void list_push(list_t* list, void* data);
list_t* list_concat(list_t* list1, list_t* list2);
bool list_contains(list_t* list, void* data, int (*compare)(void*, void*));
void list_release(list_t* list);
void list_for_each(list_t* list, void (*execute)(void*));

void list_initialize(list_t* list, void (*destructor)(void*)) {
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
   list1->tail = list2->head;
   return list1;
}

bool list_contains(list_t* list, void* data, int (*compare)(void*, void*)) {
   list_node_t* node = list->head;
   bool contains = false;
   while (node != NULL && contains == false) {
      contains = compare != NULL ? compare(node->data, data) == 0 : node->data == data;
   }
   return contains;
}

// Deallocate the list using the defined destructor (or free if not defined)
void list_release(list_t* list) {
   if (!list) {
      return;
   }
   list_for_each(list, list->destructor ? list->destructor : free);
   free(list);
}

// Traverse each element in the list
void list_for_each(list_t* list, void (*execute)(void*)) {
   list_node_t* current = list->head;
   while (current != NULL) {
      execute(current->data);
      current = current->next;
   }
}

// Traverse each element in the list
