//
// Created by Anna.Kozhemyako on 26.10.2021.
//

#ifndef GRAPH_LAB_GRAPH_H
#define GRAPH_LAB_GRAPH_H

#include <stdint.h>

#define MAX_LABEL_LENGTH 40
#define MAX_LABELS_COUNT 8

//41 bytes
typedef struct Node {
    uint8_t in_use;
    uint32_t first_rel_id;
    uint32_t first_prop_id;
    uint32_t label_id[MAX_LABELS_COUNT]; //multiple labels????
} Node;

//33 bytes
typedef struct Relation {
    uint8_t in_use;
    uint32_t from_id;
    uint32_t to_id;
    uint32_t label_id; //only one label per relation
    uint32_t from_prev_rel_id;
    uint32_t from_next_rel_id;
    uint32_t to_prev_rel_id;
    uint32_t to_next_rel_id;
    uint32_t first_prop_id;
} Relation;

//49 bytes
typedef struct Property {
    uint8_t in_use;
    uint32_t key_id;
    char value[MAX_LABEL_LENGTH];
    //type?
    uint32_t next_prop_id;
} Property;

//45 bytes
typedef struct Label {
    uint8_t in_use;
    char label[MAX_LABEL_LENGTH];
    uint32_t records_count;
} Label;

//45 bytes
typedef struct Property_Key {
    uint8_t in_use;
    char key[MAX_LABEL_LENGTH];
    uint32_t records_count;
} Property_Key;

typedef union Block {
    Node node;
    Relation relation;
    Property property;
    Property_Key property_key;
    Label label;
} Block;

#endif //GRAPH_LAB_GRAPH_H
