//
// Created by Anna.Kozhemyako on 11.11.2021.
//

#ifndef GRAPH_LAB_DATA_H
#define GRAPH_LAB_DATA_H

#include "../graph/graph.h"

typedef struct Prop_Descriptor {
    char* key;
    char* value;
    uint8_t type;
} Prop_Descriptor;

typedef struct Relation_Descriptor Relation_Descriptor;

typedef struct Node_Descriptor {
    Linked_List* labels;
    Linked_List* properties;
    Linked_List* relations;
} Node_Descriptor;

struct Relation_Descriptor {
    char* label;
    Block_Pointer first_node_id;
    Block_Pointer second_node_id;
    Linked_List* properties;
};

#endif //GRAPH_LAB_DATA_H
