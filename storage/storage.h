//
// Created by Anna.Kozhemyako on 28.10.2021.
//

#ifndef GRAPH_LAB_STORAGE_H
#define GRAPH_LAB_STORAGE_H

#include <stdio.h>

#include "../graph/graph.h"
#include "linked_list.h"
#include "data.h"

#define STORAGE_MARK 666

#define PAGES_INDEX 0

#define INIT_PAGES_COUNT 5
#define COUNTERS_AMOUNT INIT_PAGES_COUNT + 1

#define PAGE_SIZE 8192 // 8Kb

typedef struct Storage_Meta {
    uint32_t storage_mark;
    uint32_t last_block_id;
    uint32_t blocks_by_type[COUNTERS_AMOUNT];
} Storage_Meta;

typedef struct Storage_Descriptor {
    FILE* storage_file;
    Storage_Meta* meta;
    Linked_List* free_blocks_list;
} Storage_Descriptor;

uint8_t open_storage(const char* filename, Storage_Descriptor** storage_descriptor);

uint8_t close_storage(Storage_Descriptor* storage_descriptor);

uint32_t add_node(Node_Descriptor* node_descriptor, Storage_Descriptor* storage_descriptor);

uint8_t get_node(uint32_t node_id, Node_Descriptor* node_descriptor, Storage_Descriptor* storage_descriptor);

#endif //GRAPH_LAB_STORAGE_H
