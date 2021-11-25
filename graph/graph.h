//
// Created by Anna.Kozhemyako on 26.10.2021.
//

#ifndef GRAPH_LAB_GRAPH_H
#define GRAPH_LAB_GRAPH_H

#include <stdint.h>

#define MAX_LABEL_LENGTH 8
#define MAX_LABELS_COUNT 4

#define EMPTY_BLOCK 0
#define NODE_BLOCK_TYPE 1
#define RELATION_BLOCK_TYPE 2
#define PROP_BLOCK_TYPE 3
#define PROP_VALUE_BLOCK_TYPE 4
#define LABELS_BLOCK_TYPE 5

#define BLOCK_SIZE 40
#define PAYLOAD_SIZE BLOCK_SIZE - 1

//1 byte
typedef uint8_t Block_Type;

typedef uint32_t Block_Pointer;

//20 bytes
typedef struct Node {
    Block_Pointer first_rel_id;
    Block_Pointer first_prop_id;
    Block_Pointer labels_id; //multiple labels????
    Block_Pointer prev_node_id;
    Block_Pointer next_node_id;
} Node;

//32 bytes
typedef struct Relation {
    Block_Pointer from_id;
    Block_Pointer to_id;
    Block_Pointer label_id; //only one label per relation
    Block_Pointer from_prev_rel_id;
    Block_Pointer from_next_rel_id;
    Block_Pointer to_prev_rel_id;
    Block_Pointer to_next_rel_id;
    Block_Pointer first_prop_id;
} Relation;

//52 bytes
typedef struct Property {
    uint8_t type;
    char key[MAX_LABEL_LENGTH];
    Block_Pointer value_id;
    Block_Pointer next_prop_id;
} Property;

//44 bytes
typedef struct Labels {
    uint8_t labels_count;
    char labels[MAX_LABELS_COUNT][MAX_LABEL_LENGTH];
} Labels;

//40 bytes
typedef struct String_Value {
    char value[PAYLOAD_SIZE];
} String_Value;

typedef union Block {
    Node node;
    Relation relation;
    Property property;
    String_Value property_value;
    Labels labels;
} Block;

#endif //GRAPH_LAB_GRAPH_H
