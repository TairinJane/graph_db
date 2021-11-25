#include <stdlib.h>
#include <string.h>

#include "storage.h"
#include "util.h"
#include "data.h"

uint8_t init_storage(FILE* file, Storage_Meta* meta) {
    meta->storage_mark = STORAGE_MARK;
    printf("Init storage\n");

    meta->last_block_id = 0;
    meta->first_node_id = NULL;
    meta->last_node_id = NULL;

    printf("Meta init\n");

    fseek(file, 0, SEEK_SET);
    size_t count = fwrite(meta, sizeof(Storage_Meta), 1, file);
    if (count != 1) {
        printf("Failed meta write\n");
        return 1;
    }

    fflush(file);

    return 0;
}

uint8_t get_block_size_by_type(Block_Type block_type) {
    uint8_t block_size;
    switch (block_type) {
        case NODE_BLOCK_TYPE:
            block_size = sizeof(Node);
            break;
        case PROP_BLOCK_TYPE:
            block_size = sizeof(Property);
            break;
        case RELATION_BLOCK_TYPE:
            block_size = sizeof(Relation);
            break;
        case PROP_VALUE_BLOCK_TYPE:
            block_size = sizeof(String_Value);
            break;
        case LABELS_BLOCK_TYPE:
            block_size = sizeof(Labels);
            break;
        default:
            return (uint8_t) NULL;
    }
    return block_size;
}

Linked_List* find_empty_blocks(Storage_Descriptor* storage_descriptor) {

    printf("\nFind empty blocks\n");

    printf("Storage Meta in find: mark = %d, last block id = %d\n",
           storage_descriptor->meta->storage_mark,
           storage_descriptor->meta->last_block_id);

    Block_Type* block_type;
    Linked_List* empty_blocks_list = init_list();

    for (uint32_t i = 0; i <= storage_descriptor->meta->last_block_id; ++i) {
        fseek(storage_descriptor->storage_file, sizeof(Storage_Meta) + i * sizeof(Block), SEEK_SET);
        fread(&block_type, sizeof(Block_Type), 1, storage_descriptor->storage_file);
        if (block_type == EMPTY_BLOCK) {
            add_last(empty_blocks_list, (void*) i);
        }
    }

    printf("Found %d empty blocks\n", empty_blocks_list->size);
    return empty_blocks_list;
}

uint8_t open_storage(const char* filename, Storage_Descriptor** storage_descriptor) {
    printf("\nStart opening %s\n", filename);

    FILE* storage_file;
    errno_t err = fopen_s(&storage_file, filename, "wb+");

    if (err != 0) {
        printf("Storage File opening failed\n");
        fclose(storage_file);
        return 1;
    }

    printf("Read meta\n");
    Storage_Meta* storage_meta = malloc(sizeof(Storage_Meta));

    fseek(storage_file, 0, SEEK_SET);
    size_t read_res = fread(storage_meta, sizeof(Storage_Meta), 1, storage_file);
    printf("Meta read count: %d\n", read_res);

    if (read_res != 1) {
        printf("Read Storage Meta failed\n");
        init_storage(storage_file, storage_meta);
    }

    *storage_descriptor = malloc(sizeof(Storage_Descriptor));

    (*storage_descriptor)->storage_file = storage_file;
    (*storage_descriptor)->meta = storage_meta;
    (*storage_descriptor)->free_blocks_list = find_empty_blocks(*storage_descriptor);

    printf("Storage Meta init: mark = %d, last block id = %d\n",
           (*storage_descriptor)->meta->storage_mark,
           (*storage_descriptor)->meta->last_block_id);

    return 0;
}

uint8_t close_storage(Storage_Descriptor* storage_descriptor) {
    printf("\nClose storage\n");

    printf("Update meta\n");
    fseek(storage_descriptor->storage_file, 0, SEEK_SET);
    fwrite(storage_descriptor->meta, sizeof(Storage_Meta), 1, storage_descriptor->storage_file);
    fflush(storage_descriptor->storage_file);

    int res = fclose(storage_descriptor->storage_file);
    free(storage_descriptor->meta);
    printf("Free lists\n");
    free_list(storage_descriptor->free_blocks_list, 0);
    printf("Free storage\n");

    return res;
}

uint64_t get_offset_by_id(Block_Pointer id) {
    return sizeof(Storage_Meta) + (sizeof(Block_Type) + sizeof(Block)) * id;
}

uint8_t get_block_by_id(Block_Pointer id, void* block, Block_Type* block_type, Storage_Descriptor* storage_descriptor) {
    printf("Get block %d, last = %d\n", id, storage_descriptor->meta->last_block_id);

    if (id > storage_descriptor->meta->last_block_id || id < 1) {
        printf("No such block\n");
        return 1;
    }

    uint64_t offset = get_offset_by_id(id);
    fseek(storage_descriptor->storage_file, (long) offset, SEEK_SET);
    fread(block_type, sizeof(Block_Type), 1, storage_descriptor->storage_file);
    fread(block, get_block_size_by_type(*block_type), 1, storage_descriptor->storage_file);

    return 0;
}

uint8_t write_block_by_id(Block_Pointer id, void* block, Block_Type block_type, Storage_Descriptor* storage_descriptor) {
    printf("Write block id = %d, type = %d\n", id, block_type);
    uint64_t offset = get_offset_by_id(id);
    fseek(storage_descriptor->storage_file, (long) offset, SEEK_SET);
    fwrite(&block_type, sizeof(Block_Type), 1, storage_descriptor->storage_file);
    fwrite(block, sizeof(Block), 1, storage_descriptor->storage_file);
    fflush(storage_descriptor->storage_file);

    return 0;
}

Block_Pointer get_free_id(Storage_Descriptor* storage_descriptor) {
    if (storage_descriptor->free_blocks_list->size > 0) {
        return (Block_Pointer) storage_descriptor->free_blocks_list->first->value;
    } else {
        return storage_descriptor->meta->last_block_id + 1;
    }
}

Block_Pointer write_block_to_free(Block_Type block_type, void* block, Storage_Descriptor* storage_descriptor) {
    Block_Pointer free_block_id;

    if (storage_descriptor->free_blocks_list->size > 0) {
        free_block_id = (Block_Pointer) storage_descriptor->free_blocks_list->first->value;
        remove_first(storage_descriptor->free_blocks_list);
    } else {
        free_block_id = ++storage_descriptor->meta->last_block_id;
    }

    write_block_by_id(free_block_id, block, block_type, storage_descriptor);

    return free_block_id;
}

uint8_t delete_block_by_id(Block_Pointer id, Storage_Descriptor* storage_descriptor) {
    Block_Type empty = (Block_Type) EMPTY_BLOCK;
    printf("Delete block id = %d\n", id);

    fseek(storage_descriptor->storage_file, (long) get_offset_by_id(id), SEEK_SET);
    fwrite(&empty, sizeof(Block_Type), 1, storage_descriptor->storage_file);
    fflush(storage_descriptor->storage_file);

    add_last(storage_descriptor->free_blocks_list, (void*) id);

    return 0;
}

Block_Pointer add_properties(Linked_List* properties, Storage_Descriptor* storage_descriptor) {
    Block_Pointer first_prop_id = (Block_Pointer) NULL;

    if (properties->size > 0) {
        List_Node* current_prop = properties->first;
        Block_Pointer current_id;

        Prop_Descriptor* prop_descriptor;
        Property* prop;

        Block_Pointer prev_prop_id = (Block_Pointer) NULL;
        Property* prev_prop;

        for (uint32_t i = 0; i < properties->size; ++i) {
            prop = malloc(sizeof(Property));

            prop_descriptor = current_prop->value;
            printf("\nAdd property %s, key len = %d, max len = %d\n", prop_descriptor->key,
                   strlen(prop_descriptor->key),
                   strlen(prop->key));

            prop->type = prop_descriptor->type;
            strncpy_s(prop->key, strnlen_s(prop->key, MAX_LABEL_LENGTH), prop_descriptor->key, _TRUNCATE);
            prop->value_id = write_block_to_free(PROP_VALUE_BLOCK_TYPE, prop_descriptor->value, storage_descriptor);
            prop->next_prop_id = (Block_Pointer) NULL;

            current_id = write_block_to_free(PROP_BLOCK_TYPE, prop, storage_descriptor);

            if (prev_prop_id != (Block_Pointer) NULL) {
                prev_prop->next_prop_id = current_id;
                write_block_by_id(prev_prop_id, prev_prop, PROP_BLOCK_TYPE, storage_descriptor);
            }

            printf("Property %s = %d, type = %d, next = %d\n", prop->key, prop->value_id, prop->type,
                   prop->next_prop_id);

            if (current_prop == properties->first) {
                first_prop_id = current_id;
            }

            current_prop = current_prop->next;
            prev_prop_id = current_id;
            prev_prop = prop;
        }

        fflush(storage_descriptor->storage_file);
    }

    return first_prop_id;
}

Block_Pointer get_next_rel_id(Block_Pointer node_id, Relation* relation) {
    if (relation->from_id == node_id) {
        return relation->from_next_rel_id;
    }
    return relation->to_next_rel_id;
}

Block_Pointer get_prev_rel_id(Block_Pointer node_id, Relation* relation) {
    if (relation->from_id == node_id) {
        return relation->from_prev_rel_id;
    }
    return relation->to_prev_rel_id;
}

Block_Pointer find_last_relation_id(Block_Pointer node_id, Storage_Descriptor* storage_descriptor) {
    Node node;
    Block_Type block_type;
    get_block_by_id(node_id, &node, &block_type, storage_descriptor);

    Block_Pointer relation_id = node.first_rel_id;

    if (relation_id != (Block_Pointer) NULL) {
        Block_Pointer next_relation_id = (Block_Pointer) NULL;
        Relation relation;

        uint8_t found = 0;
        do {
            get_block_by_id(next_relation_id, &relation, &block_type, storage_descriptor);

            next_relation_id = get_next_rel_id(node_id, &relation);

            if (next_relation_id == (Block_Pointer) NULL) {
                found = 1;
            } else {
                relation_id = next_relation_id;
            }
        }
        while (found == 0);
    }

    return relation_id;
}

Block_Pointer add_relations(Linked_List* relations_list, Storage_Descriptor* storage_descriptor) {

    Block_Pointer first_rel_id = (Block_Pointer) NULL;
    if (relations_list->size > 0) {

        List_Node* current_rel_node = relations_list->first;
        Relation_Descriptor* rel_descriptor;

        Relation relation;
        String_Value rel_label;
        Block_Pointer rel_id;

        for (uint32_t i = 0; i < relations_list->size; ++i) {

            rel_descriptor = current_rel_node->value;
            printf("Write relation from %d to %d\n", rel_descriptor->first_node_id, rel_descriptor->second_node_id);

            relation.first_prop_id = add_properties(rel_descriptor->properties, storage_descriptor);

            relation.from_id = rel_descriptor->first_node_id;
            relation.from_prev_rel_id = find_last_relation_id(rel_descriptor->first_node_id, storage_descriptor);
            relation.from_next_rel_id = (Block_Pointer) NULL;

            relation.to_id = rel_descriptor->second_node_id;
            relation.to_prev_rel_id = find_last_relation_id(rel_descriptor->second_node_id, storage_descriptor);
            relation.to_next_rel_id = (Block_Pointer) NULL;

            strncpy_s(rel_label.value, strnlen_s(rel_label.value, MAX_LABEL_LENGTH), rel_descriptor->label, _TRUNCATE);
            relation.label_id = write_block_to_free(PROP_VALUE_BLOCK_TYPE, &rel_label, storage_descriptor);

            rel_id = write_block_to_free(RELATION_BLOCK_TYPE, &relation, storage_descriptor);

            if (first_rel_id == (Block_Pointer) NULL) {
                first_rel_id = rel_id;
            }

            current_rel_node = current_rel_node->next;
        }

        fflush(storage_descriptor->storage_file);
    }

    return first_rel_id;
}

uint8_t add_node_to_list(Block_Pointer added_node_id, Storage_Descriptor * storage_descriptor) {
    Block_Pointer last_node_id = storage_descriptor->meta->last_node_id;
    Node last_node;
    Block_Type block_type;

    get_block_by_id(last_node_id, &last_node, &block_type, storage_descriptor);

    last_node.next_node_id = added_node_id;
    write_block_by_id(last_node_id, &last_node, block_type, storage_descriptor);

    storage_descriptor->meta->last_node_id = added_node_id;

    fflush(storage_descriptor->storage_file);

    return 0;
}

Block_Pointer add_node(Node_Descriptor* node_descriptor, Storage_Descriptor* storage_descriptor) {
    Node node;
    printf("\nAdd node\n");

    Block_Pointer first_property_id = (Block_Pointer) NULL;
    if (node_descriptor->properties != NULL) {
        first_property_id = add_properties(node_descriptor->properties, storage_descriptor);
    }
    node.first_prop_id = first_property_id;

    Block_Pointer first_rel_id = (Block_Pointer) NULL;
    if (node_descriptor->relations != NULL) {
        first_rel_id = add_relations(node_descriptor->relations, storage_descriptor);
    }
    node.first_rel_id = first_rel_id;

    node.labels_id = (Block_Pointer) NULL;
    if (node_descriptor->labels != NULL) {
        char* label;
        Labels node_labels;
        List_Node* current_label_node = node_descriptor->labels->first;

        node_labels.labels_count = node_descriptor->labels->size;

        for (uint32_t i = 0; i < node_descriptor->labels->size && i < MAX_LABELS_COUNT; ++i) {
            label = current_label_node->value;
            printf("Add label %s, len = %d\n", label, strnlen_s(label, MAX_LABEL_LENGTH));
            strncpy_s(node_labels.labels[i], strnlen_s(node_labels.labels[i], MAX_LABEL_LENGTH), label, _TRUNCATE);

            current_label_node = current_label_node->next;
        }
        node.labels_id = write_block_to_free(LABELS_BLOCK_TYPE, &node_labels, storage_descriptor);
    }
    node.next_node_id = (Block_Pointer) NULL;
    node.prev_node_id = storage_descriptor->meta->last_node_id;

    Block_Pointer node_id = write_block_to_free(NODE_BLOCK_TYPE, &node, storage_descriptor);

    add_node_to_list(node_id, storage_descriptor);

    return node_id;
}

uint8_t get_node_labels(Block_Pointer labels_id, Linked_List* labels_list, Storage_Descriptor* storage_descriptor) {
    Labels node_labels;
    Block_Type block_type;
    printf("\nGel labels\n");

    if (labels_id != (Block_Pointer) NULL) {
        char* label;
            get_block_by_id(labels_id, &node_labels, &block_type, storage_descriptor);
            for (int i = 0; i < node_labels.labels_count; ++i) {
                label = malloc(MAX_LABEL_LENGTH);
                strncpy_s(label, MAX_LABEL_LENGTH, node_labels.labels[i], _TRUNCATE);
                printf("Get label %d = %s\n", i, label);
                add_last(labels_list, label);
            }
    }

    return 0;
}

uint8_t
get_properties(Block_Pointer first_property_id, Linked_List* properties_list, Storage_Descriptor* storage_descriptor) {
    printf("\nGet props start %d\n", first_property_id);

    Prop_Descriptor* prop_descriptor;
    Property property;
    Block_Type block_type;
    String_Value property_value;

    Block_Pointer prop_id = first_property_id;

    while (prop_id != (Block_Pointer) NULL) {
        printf("\nGet prop %d\n", prop_id);

        get_block_by_id(prop_id, &property, &block_type, storage_descriptor);
        printf("Prop id = %d, next prop = %d, type = %d\n", prop_id, property.next_prop_id, property.type);

        prop_descriptor = malloc(sizeof(Prop_Descriptor));

        printf("Get prop key\n");
        prop_descriptor->key = malloc(MAX_LABEL_LENGTH);
        strncpy_s(prop_descriptor->key, MAX_LABEL_LENGTH, property.key, _TRUNCATE);

        printf("Get prop value\n");
        get_block_by_id(property.value_id, &property_value, &block_type, storage_descriptor);
        prop_descriptor->value = malloc(PAYLOAD_SIZE);
        strncpy_s(prop_descriptor->value, PAYLOAD_SIZE, property_value.value, _TRUNCATE);

        prop_descriptor->type = property.type;

        printf("Prop: %s = %s\n", prop_descriptor->key, prop_descriptor->value);

        add_last(properties_list, prop_descriptor);

        prop_id = property.next_prop_id;
    }

    return 0;
}

uint8_t get_node_relations(Block_Pointer node_id, Block_Pointer first_rel_id, Linked_List* relations_list,
                           Storage_Descriptor* storage_descriptor) {
    if (first_rel_id != (Block_Pointer) NULL) {

        Block_Pointer relation_id = first_rel_id;
        Relation relation;
        Block_Type block_type;

        Relation_Descriptor* relation_descriptor;
        Linked_List* prop_list;

        do {
            get_block_by_id(relation_id, &relation, &block_type, storage_descriptor);

            if (block_type != EMPTY_BLOCK) {

                relation_descriptor = malloc(sizeof(Relation_Descriptor));
                relation_descriptor->first_node_id = relation.from_id;
                relation_descriptor->second_node_id = relation.to_id;

                String_Value label;
                get_block_by_id(relation.label_id, &label, &block_type, storage_descriptor);
                strncpy_s(relation_descriptor->label, strnlen_s(relation_descriptor->label, PAYLOAD_SIZE), label.value,
                          _TRUNCATE);

                prop_list = init_list();
                get_properties(relation.first_prop_id, prop_list, storage_descriptor);
                relation_descriptor->properties = prop_list;

                add_last(relations_list, relation_descriptor);

                relation_id = get_next_rel_id(node_id, &relation);
            } else relation_id = (Block_Pointer) NULL;
        }
        while (relation_id != (Block_Pointer) NULL);
    }

    return 0;
}

uint8_t get_node(Block_Pointer node_id, Node_Descriptor* node_descriptor, Storage_Descriptor* storage_descriptor) {
    printf("\nGet node %d\n", node_id);
    Node* node = malloc(sizeof(Node));
    Block_Type* block_type = malloc(sizeof(Block_Type));
    get_block_by_id(node_id, node, block_type, storage_descriptor);

    node_descriptor->properties = init_list();
    get_properties(node->first_prop_id, node_descriptor->properties, storage_descriptor);

    node_descriptor->relations = init_list();
    get_node_relations(node_id, node->first_rel_id, node_descriptor->relations, storage_descriptor);

    node_descriptor->labels = init_list();
    get_node_labels(node->labels_id, node_descriptor->labels, storage_descriptor);

    return 0;
}

uint8_t delete_node_relations(Block_Pointer node_id, Block_Pointer first_rel_id, Storage_Descriptor* storage_descriptor) {
    Block_Type block_type;
    Relation relation;
    Relation prev_relation;

    Block_Pointer relation_id = first_rel_id;

    if (relation_id != (Block_Pointer) NULL) {
        Block_Pointer next_relation_id;
        Block_Pointer prev_relation_id;
        Block_Pointer deleted_rel_id = (Block_Pointer) NULL;

        do {
            get_block_by_id(relation_id, &relation, &block_type, storage_descriptor);

            if (block_type != EMPTY_BLOCK) {

                next_relation_id = get_next_rel_id(node_id, &relation);
                prev_relation_id = get_prev_rel_id(node_id, &relation);

                if (prev_relation_id != deleted_rel_id) {
                    get_block_by_id(prev_relation_id, &prev_relation, &block_type, storage_descriptor);

                    if (block_type != EMPTY_BLOCK) {
                        if (relation.from_id == node_id) {
                            prev_relation.from_next_rel_id = next_relation_id;
                        } else if (relation.to_id == node_id) {
                            prev_relation.to_next_rel_id = next_relation_id;
                        }
                        write_block_by_id(prev_relation_id, &prev_relation, block_type, storage_descriptor);
                    }
                }

                deleted_rel_id = relation_id;
                relation_id = next_relation_id;
            } else relation_id = (Block_Pointer) NULL;
        }
        while (relation_id != (Block_Pointer) NULL);
    }

    return 0;
}

uint8_t delete_properties(Block_Pointer first_property_id, Storage_Descriptor* storage_descriptor) {
    Block_Pointer current_id = first_property_id;

    Block_Type block_type;
    Property prop;

    while (current_id != (Block_Pointer) NULL) {
        get_block_by_id(current_id, &prop, &block_type, storage_descriptor);

        if (block_type == PROP_BLOCK_TYPE) {
            current_id = prop.next_prop_id;
            delete_block_by_id(current_id, storage_descriptor);
        } else {
            current_id = (Block_Pointer) NULL;
        }
    }

    return 0;
}
