//
// Created by Anna.Kozhemyako on 11.11.2021.
//

#include "linked_list.h"

#include <stddef.h>
#include <string.h>
#include <malloc.h>

void* get_element_by_index(Linked_List* ptr, uint32_t index) {
    if (index >= ptr->size) {
        return NULL;
    }
    List_Node* current_node = ptr->first;
    for (uint32_t i = 0; i < index; i++) {
        current_node = current_node->next;
    }
    return current_node->value;
}

uint32_t add_first(Linked_List* ptr, void* value) {
    List_Node* new_first = malloc(sizeof(List_Node));
    new_first->next = ptr->first;
    new_first->value = value;
    ptr->first->prev = new_first;
    ptr->first = new_first;
    return 0;
}

uint32_t add_last(Linked_List* ptr, void* value) {
    List_Node* new_last = malloc(sizeof(List_Node));
    new_last->prev = ptr->last;
    new_last->value = value;
    if (ptr->last == NULL) {
        ptr->last = new_last;
        ptr->first = new_last;
    } else {
        ptr->last->next = new_last;
        ptr->last = new_last;
    }
    return ++ptr->size;
}

Linked_List* init_list() {
    Linked_List* list = malloc(sizeof(Linked_List));
    list->first = NULL;
    list->last = NULL;
    list->size = 0;
    return list;
}

void free_list(Linked_List* ptr, bool is_malloc_value) {
    if (ptr->size != 0) {
        List_Node* current = ptr->first;
        for (uint32_t i = 1; i < ptr->size; ++i) {
            current = current->next;
            if (is_malloc_value) free(current->prev->value);
            free(current->prev);
        }
        if (is_malloc_value) free(current->value);
        free(current);
    }
    free(ptr);
}

void remove_first(Linked_List* list) {
    List_Node* next_first = list->first->next;
    free(list->first);
    list->size--;
    list->first = next_first;
}

void remove_element_by(bool (* by)(void*, char*, char*), Linked_List* list, char* first_to_find, char* second_to_find) {
    List_Node* current = list->first;
    while (current != NULL && !by(current->value, first_to_find, second_to_find)) {
        current = current->next;
    }
    if (current != NULL) {
        if (list->first == current) {
            list->first = current->next;
        }
        if (list->last == current) {
            list->last = current->prev;
        }
        if (current->prev != NULL) {
            current->prev->next = current->next;
        }
        if (current->next != NULL) {
            current->next->prev = current->prev;
        }
        list->size--;
        free(current);
    }
}

void* find_element_by(bool (* by)(void*, char*, char*), Linked_List* list, char* first_to_find, char* second_to_find) {
    if (list->size == 0) {
        return NULL;
    }
    List_Node* current = list->first;
    while (current != NULL && !by(current->value, first_to_find, second_to_find)) {
        current = current->next;
    }
    if (current == NULL) return NULL;
    return current->value;
}

bool equals_by_string_value(void* value, char* to_find, char* second_argument) {
    return strcmp(value, to_find) == 0;
}

uint16_t get_last_n(Linked_List* list, void** buffer, uint16_t buffer_size, bool(* filter)(void*, char* to_filter),
        char* to_filter) {
    List_Node* current = list->last;
    for (uint16_t i = 0; i < buffer_size; ++i) {
        if (current == NULL) return i;
        if (filter == NULL || filter(current->value, to_filter)) {
            buffer[i] = current->value;
        } else {
            --i;
        }
        current = current->prev;
    }
    return buffer_size;
}