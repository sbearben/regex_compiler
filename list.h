#ifndef LIST_H
#define LIST_H

#include <stdbool.h>

#define list_traverse(list, node) for (node = list->head; node != NULL; node = node->next)

typedef struct list list_t;
typedef struct list_node list_node_t;

struct list {
      list_node_t* head;
      list_node_t* tail;
      void (*destructor)(list_node_t*);
};

struct list_node {
      void* data;
      struct list_node* next;
};

void list_initialize(list_t* list, void (*destructor)(list_node_t*));
int list_size(list_t* list);
bool list_empty(list_t* list);
void list_push(list_t* list, void* data);
// void list_add_head(list_t* list, void* data);
list_t* list_concat(list_t* list1, list_t* list2);
void* list_find(list_t* list, void* data, int (*compare)(void*, void*));
bool list_contains(list_t* list, void* data, int (*compare)(void*, void*));
void list_sort(list_t* list, int (*compare)(void*, void*));
void list_release(list_t* list);
void list_default_destructor(list_node_t*);
void list_noop_data_destructor(list_node_t* node);

#endif  // LIST_H
