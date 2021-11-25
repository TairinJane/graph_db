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

typedef struct Storage_Meta {
    uint32_t storage_mark;
    Block_Pointer last_block_id;
    Block_Pointer first_node_id;
    Block_Pointer last_node_id;
} Storage_Meta;

typedef struct Storage_Descriptor {
    FILE* storage_file;
    Storage_Meta* meta;
    Linked_List* free_blocks_list;
} Storage_Descriptor;

uint8_t open_storage(const char* filename, Storage_Descriptor** storage_descriptor);

uint8_t close_storage(Storage_Descriptor* storage_descriptor);

Block_Pointer add_node(Node_Descriptor* node_descriptor, Storage_Descriptor* storage_descriptor);

uint8_t get_node(Block_Pointer node_id, Node_Descriptor* node_descriptor, Storage_Descriptor* storage_descriptor);

#endif //GRAPH_LAB_STORAGE_H
