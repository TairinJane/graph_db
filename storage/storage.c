#include <stdlib.h>
#include <string.h>

#include "storage.h"
#include "util.h"
#include "data.h"

uint8_t init_storage(FILE* file, Storage_Meta* meta) {
    meta->storage_mark = STORAGE_MARK;
    printf("Init storage\n");

    for (int i = 0; i < LABELS_BLOCK_TYPE + 1; ++i) {
        meta->blocks_by_type[i] = 0;
    }

    meta->last_block_id = 0;

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

uint8_t get_block_size_by_type(uint8_t block_type) {
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
            block_size = sizeof(Property_Value);
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

    printf("Storage Meta in find: mark = %d, last block id = %d, blocks = %d\n",
            storage_descriptor->meta->storage_mark,
            storage_descriptor->meta->last_block_id,
            storage_descriptor->meta->blocks_by_type[PAGES_INDEX]);

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
    errno_t err = fopen_s(&storage_file, filename, "rb+");

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

    printf("Storage Meta init: mark = %d, last block id = %d, blocks = %d\n",
            (*storage_descriptor)->meta->storage_mark,
            (*storage_descriptor)->meta->last_block_id,
            (*storage_descriptor)->meta->blocks_by_type[PAGES_INDEX]);

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

uint64_t get_offset_by_id(uint32_t id) {
    return sizeof(Storage_Meta) + (sizeof(Block_Type) + sizeof(Block)) * id;
}

uint8_t get_block_by_id(uint32_t id, void* block, Block_Type* block_type, Storage_Descriptor* storage_descriptor) {
    if (id > storage_descriptor->meta->last_block_id) {
        return 1;
    }

    uint64_t offset = get_offset_by_id(id);
    fseek(storage_descriptor->storage_file, (long) offset, SEEK_SET);
    fread(block_type, sizeof(Block_Type), 1, storage_descriptor->storage_file);
    fread(block, sizeof(Block), 1, storage_descriptor->storage_file);

    return 0;
}

uint32_t write_block_to_free(Block_Type block_type, void* block, Storage_Descriptor* storage_descriptor) {
    uint32_t free_block_id;

    if (storage_descriptor->free_blocks_list->size > 0) {
        free_block_id = (uint32_t) storage_descriptor->free_blocks_list->first->value;
        remove_first(storage_descriptor->free_blocks_list);
    } else {
        free_block_id = ++(storage_descriptor->meta->last_block_id);
    }

    uint64_t offset = get_offset_by_id(free_block_id);

    fseek(storage_descriptor->storage_file, (long) offset, SEEK_SET);
    fwrite(&block_type, sizeof(Block_Type), 1, storage_descriptor->storage_file);
    fwrite(&block, sizeof(Block), 1, storage_descriptor->storage_file);
    fflush(storage_descriptor->storage_file);

    return free_block_id;
}

uint8_t delete_block_by_id(uint32_t id, Storage_Descriptor* storage_descriptor) {
    Block_Type empty = (Block_Type) EMPTY_BLOCK;

    fseek(storage_descriptor->storage_file, (long) get_offset_by_id(id), SEEK_SET);
    fwrite(&empty, sizeof(Block_Type), 1, storage_descriptor->storage_file);
    fflush(storage_descriptor->storage_file);

    add_last(storage_descriptor->free_blocks_list, (void*) id);

    return 0;
}

uint8_t delete_properties(uint32_t first_property_id, Storage_Descriptor* storage_descriptor) {
    uint32_t current_id = first_property_id;

    Block_Type block_type;
    Property prop;

    while (current_id != (uint32_t) NULL) {
        get_block_by_id(current_id, &prop, &block_type, storage_descriptor);

        if (block_type == PROP_BLOCK_TYPE) {
            current_id = prop.next_prop_id;
            delete_block_by_id(current_id, storage_descriptor);
        } else {
            current_id = (uint32_t) NULL;
        }
    }

    return 0;
}

uint32_t add_properties(Linked_List* properties, Storage_Descriptor* storage_descriptor) {
    uint32_t first_prop_id = (uint32_t) NULL;

    if (properties->size > 0) {
        List_Node* current_prop = properties->first;
        uint32_t current_id;

        Prop_Descriptor* prop_descriptor;
        Property prop;

        for (uint32_t i = 0; i < properties->size; ++i) {
            prop_descriptor = current_prop->value;

            prop.type = prop_descriptor->type;
            strcat_s(prop.key, strnlen_s(prop_descriptor->key, MAX_LABEL_LENGTH), prop_descriptor->key);
            prop.value_id = write_block_to_free(PROP_VALUE_BLOCK_TYPE, prop_descriptor->value, storage_descriptor);

            current_id = write_block_to_free(PROP_BLOCK_TYPE, &prop, storage_descriptor);

            if (i == 0) {
                first_prop_id = current_id;
            }

            current_prop = current_prop->next;
        }

        fflush(storage_descriptor->storage_file);
    }

    return first_prop_id;
}

uint32_t find_last_relation_id(uint32_t node_id, Storage_Descriptor* storage_descriptor) {
    Node node;
    Block_Type block_type;
    get_block_by_id(node_id, &node, &block_type, storage_descriptor);

    uint32_t relation_id = node.first_rel_id;

    if (relation_id != (uint32_t) NULL) {
        uint32_t next_relation_id = (uint32_t) NULL;
        Relation relation;

        uint8_t found = 0;
        do {
            get_block_by_id(next_relation_id, &relation, &block_type, storage_descriptor);

            if (relation.from_id == node_id) {
                next_relation_id = relation.from_next_rel_id;
            } else if (relation.to_id == node_id) {
                next_relation_id = relation.to_next_rel_id;
            }

            if (next_relation_id == (uint32_t) NULL) {
                found = 1;
            } else {
                relation_id = next_relation_id;
            }
        }
        while (found == 0);
    }

    return relation_id;
}

//TODO
uint8_t add_relations(Linked_List* relations_list, Storage_Descriptor* storage_descriptor) {
    Linked_List* free_rel_blocks_list = storage_descriptor->free_blocks_list;

    uint32_t first_property_id;
    if (relations_list->size > 0) {
        List_Node* current_free_node = free_rel_blocks_list->first;

        List_Node* current_rel_node = relations_list->first;
        Relation_Descriptor* rel_descriptor;

        for (uint32_t i = 0; i < relations_list->size; ++i) {

            rel_descriptor = current_rel_node->value;

            Relation relation;

            first_property_id = add_properties(rel_descriptor->properties, storage_descriptor);
            relation.first_prop_id = first_property_id;

            relation.from_id = rel_descriptor->first_node_id;
            relation.from_prev_rel_id = find_last_relation_id(rel_descriptor->first_node_id, storage_descriptor);
            relation.from_next_rel_id = (uint32_t) NULL;

            relation.to_id = rel_descriptor->second_node_id;
            relation.to_prev_rel_id = find_last_relation_id(rel_descriptor->second_node_id, storage_descriptor);
            relation.to_next_rel_id = (uint32_t) NULL;

            uint32_t offset = (uint32_t) current_free_node->value;
            fseek(storage_descriptor->storage_file, (long) offset, SEEK_SET);
            fwrite(&relation, sizeof(relation), 1, storage_descriptor->storage_file);

            current_rel_node = current_rel_node->next;
            current_free_node = current_free_node->next;

            remove_first(free_rel_blocks_list);
        }

        fflush(storage_descriptor->storage_file);
    }

    return relations_list->size;
}

uint32_t add_node(Node_Descriptor* node_descriptor, Storage_Descriptor* storage_descriptor) {
    Node node;

    uint32_t label_id;
    char* label;
    if (node_descriptor->labels != NULL) {
        List_Node* current_label_node = node_descriptor->labels->first;

        for (uint32_t i = 0; i < node_descriptor->labels->size; ++i) {
            label = current_label_node->value;

            /*if (label != NULL) {
                label_id = find_label_id(label, LABELS_BLOCK_TYPE, storage_descriptor);
                if (label_id == (uint32_t) NULL) {
                    label_id = add_label(label, LABELS_BLOCK_TYPE, storage_descriptor);
                }
                node.first_labels_id = label_id;
            }*/

            current_label_node = current_label_node->next;
        }

    }

    uint32_t first_property_id = (uint32_t) NULL;
    if (node_descriptor->properties != NULL) {
        first_property_id = add_properties(node_descriptor->properties, storage_descriptor);
    }
    node.first_prop_id = first_property_id;

    uint32_t first_rel_id = (uint32_t) NULL;
    if (node_descriptor->relations != NULL) {
        first_rel_id = add_relations(node_descriptor->relations, storage_descriptor);
    }
    node.first_rel_id = first_rel_id;

    uint32_t free_node_block = (uint32_t) storage_descriptor->free_blocks_list->first->value;
    fseek(storage_descriptor->storage_file, (long) free_node_block, SEEK_SET);
    fwrite(&node, sizeof(node), 1, storage_descriptor->storage_file);

    fflush(storage_descriptor->storage_file);

    return 0;
}

