//
// Created by Anna.Kozhemyako on 11.11.2021.
//

#ifndef GRAPH_LAB_LINKED_LIST_H
#define GRAPH_LAB_LINKED_LIST_H

#include <stdint.h>
#include <stdbool.h>

typedef struct List_Node List_Node;

struct List_Node {
    struct List_Node* prev;
    void* value;
    struct List_Node* next;
};

typedef struct Linked_List {
    List_Node* first;
    List_Node* last;
    uint32_t size;

    void (* free_value_fun)(void* value);
} Linked_List;

void* get_element_by_index(Linked_List* ptr, uint32_t index);

uint32_t add_first(Linked_List* ptr, void* value);

uint32_t add_last(Linked_List* ptr, void* value);

bool equals_by_string_value(void* value, char* to_find, char* second_argument);

bool by_key(void* value, char* key, char* second_value);

Linked_List* init_list();

void free_list(Linked_List* ptr, bool is_alloc_value);

void remove_element_by(bool (* by)(void*, char*, char*), Linked_List* list, char* first_to_find, char* second_to_find);

void* find_element_by(bool (* by)(void*, char*, char*), Linked_List* list, char* first_to_find, char* second_to_find);

uint16_t get_last_n(Linked_List* list, void** buffer, uint16_t buffer_size, bool(* filter)(void*, char* to_filter),
        char* to_filter);

void remove_first(Linked_List* list);

#endif //GRAPH_LAB_LINKED_LIST_H
