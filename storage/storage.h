//
// Created by Anna.Kozhemyako on 28.10.2021.
//

#ifndef GRAPH_LAB_STORAGE_H
#define GRAPH_LAB_STORAGE_H

#include <stdio.h>

#include "../graph/graph.h"
#include "linked_list.h"

#define STORAGE_MARK 66

#define PAGES_INDEX 0

#define NODE_BLOCK_TYPE 1
#define RELATION_BLOCK_TYPE 2
#define PROP_BLOCK_TYPE 3
#define KEYS_BLOCK_TYPE 4
#define LABELS_BLOCK_TYPE 5

#define INIT_PAGES_COUNT 5
#define COUNTERS_AMOUNT INIT_PAGES_COUNT + 1

#define PAGE_SIZE 8192 // 8Kb

typedef struct Storage_Meta {
    uint8_t storage_mark;
    uint32_t first_page_offsets[COUNTERS_AMOUNT];
    uint32_t pages_by_type[COUNTERS_AMOUNT];
} Storage_Meta;

typedef struct Page_Meta {
    uint8_t page_type; //nodes, rel, props, keys...
    uint16_t page_number;
    uint8_t is_full;
    uint32_t next_page_offset;
} Page_Meta;

typedef struct Storage_Descriptor {
    FILE* storage_file;
    Storage_Meta* meta;
    Linked_List* free_blocks_of_type[COUNTERS_AMOUNT];
} Storage_Descriptor;

typedef struct Free_Block {
    uint32_t offset;
    uint32_t id;
} Free_Block;

uint8_t open_storage(const char* filename, Storage_Descriptor** storage_descriptor);

uint8_t close_storage(Storage_Descriptor* storage_descriptor);

#endif //GRAPH_LAB_STORAGE_H
