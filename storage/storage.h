//
// Created by Anna.Kozhemyako on 28.10.2021.
//

#ifndef GRAPH_LAB_STORAGE_H
#define GRAPH_LAB_STORAGE_H

#include "../graph/graph.h"
#include <stdio.h>

#define STORAGE_MARK 66

typedef struct First_Page_Offsets {
    uint32_t nodes;
    uint32_t relations;
    uint32_t props;
    uint32_t labels;
    uint32_t keys;
} First_Page_Offsets;

typedef struct Storage_Counters {
    uint32_t pages_count;
    uint32_t nodes_count;
    uint32_t relations_count;
    uint32_t props_count;
    uint32_t labels_count;
    uint32_t keys_count;
} Storage_Counters;

typedef struct Storage_Meta {
    uint8_t storage_mark;
    First_Page_Offsets first_page_offsets;
    Storage_Counters counters; //counters for each data type?
} Storage_Meta;

#define NODE_BLOCK_TYPE 1
#define RELATION_BLOCK_TYPE 2
#define PROP_BLOCK_TYPE 3
#define KEYS_BLOCK_TYPE 4
#define LABELS_BLOCK_TYPE 5

#define INIT_PAGES_COUNT 5

#define RECORDS_PER_PAGE 150
#define PAGE_SIZE 8192 // 8Kb

typedef struct Page_Meta {
    uint8_t page_type; //nodes, rel, props, keys...
    uint16_t page_number;
    //empty blocks bit array???
    uint8_t is_full;
    uint32_t next_page_offset;
} Page_Meta;

typedef struct Storage_Descriptor {
    FILE* storage_file;
    Storage_Meta* meta;
} Storage_Descriptor;

typedef struct Label_Descriptor {
    char* value;
} Label_Descriptor;

typedef struct Prop_Descriptor {
    char* key;
    char* value;
} Prop_Descriptor;

typedef struct Relation_Descriptor Relation_Descriptor;

typedef struct Node_Descriptor {
    Label_Descriptor* labels;
    Prop_Descriptor* properties;
    Relation_Descriptor* relation;
    // ????((((( chto peredat?(
} Node_Descriptor;

typedef struct Relation_Descriptor {
    char* label;
    Prop_Descriptor* properties;
    Node_Descriptor* second_node;
} Relation_Descriptor;

uint8_t open_storage(const char* filename, Storage_Descriptor** storage_descriptor);

uint32_t find_empty_block(uint8_t block_type, Storage_Descriptor* storage_descriptor);

uint8_t close_storage(Storage_Descriptor* storage_descriptor);

#endif //GRAPH_LAB_STORAGE_H
