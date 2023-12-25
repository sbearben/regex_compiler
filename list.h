#ifndef LIST_H
#define LIST_H

#include <stdbool.h>

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
void list_noop_data_destructor(list_node_t* node);
void list_for_each(list_t* list, void (*execute)(list_node_t*));

#endif  // LIST_H
