#include <stdlib.h>
#include <string.h>

#include "storage.h"
#include "util.h"
#include "data.h"

uint8_t new_page(FILE* file, uint32_t offset, uint32_t page_number, uint8_t page_type) {
    if (file != NULL) {
        if (!fseek(file, (long) offset, SEEK_SET)) {
            Page_Meta page_meta;
            page_meta.is_full = 0;
            page_meta.next_page_offset = (uint32_t) NULL;
            page_meta.page_number = page_number;
            page_meta.page_type = page_type;

            printf("Write page type = %d at %d\n", page_meta.page_type, offset);

            size_t count = fwrite(&page_meta, sizeof(Page_Meta), 1, file);
            if (count != 1) {
                printf("Failed page write\n");
                return 1;
            }
            return 0;
        }
    }
    return 1;
}

uint8_t init_storage(FILE* file, Storage_Meta* meta) {
    meta->storage_mark = STORAGE_MARK;
    printf("Init storage\n");

    //add pages for every type
    uint32_t page_offset = sizeof(Storage_Meta);
    uint8_t res;
    for (int page_type = NODE_BLOCK_TYPE; page_type <= INIT_PAGES_COUNT; ++page_type) {
        res = new_page(file, page_offset, 1, page_type);
        if (res == 0) {
            meta->first_page_offsets[page_type] = page_offset;
        } else return 1;
        page_offset += PAGE_SIZE;
    }
    void* null = NULL;
    fwrite(&null, 1, 1, file);

    printf("Pages created\n");

    meta->pages_by_type[PAGES_INDEX] = INIT_PAGES_COUNT;
    meta->pages_by_type[NODE_BLOCK_TYPE] = 1;
    meta->pages_by_type[RELATION_BLOCK_TYPE] = 1;
    meta->pages_by_type[PROP_BLOCK_TYPE] = 1;
    meta->pages_by_type[LABELS_BLOCK_TYPE] = 1;
    meta->pages_by_type[KEYS_BLOCK_TYPE] = 1;

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
        case KEYS_BLOCK_TYPE:
            block_size = sizeof(Property_Key);
            break;
        case LABELS_BLOCK_TYPE:
            block_size = sizeof(Label);
            break;
        default:
            return (uint8_t) NULL;
    }
    return block_size;
}

Linked_List* find_empty_blocks(uint8_t block_type, Storage_Descriptor* storage_descriptor) {

    if (block_type > INIT_PAGES_COUNT) {
        printf("Invalid block type = %d\n", block_type);
        return NULL;
    }

    printf("\nFind empty blocks\n");

    printf("Storage Meta in find: mark = %d, first nodes page offset = %d, pages = %d\n",
            storage_descriptor->meta->storage_mark,
            storage_descriptor->meta->first_page_offsets[NODE_BLOCK_TYPE],
            storage_descriptor->meta->pages_by_type[PAGES_INDEX]);

    uint32_t start_offset = storage_descriptor->meta->first_page_offsets[block_type];
    uint8_t block_size = get_block_size_by_type(block_type);

    printf("Page Start offset = %d, block size = %d, block type = %d\n", start_offset, block_size, block_type);

    Page_Meta page_meta;
    fseek(storage_descriptor->storage_file, (long) start_offset, SEEK_SET);
    fread(&page_meta, sizeof(Page_Meta), 1, storage_descriptor->storage_file);

    printf("Page meta (%d): page type = %d, page number = %d, next = %d\n", sizeof(Page_Meta), page_meta.page_type,
            page_meta.page_number, page_meta.next_page_offset);

    Block* read_block = malloc(block_size);
    fpos_t empty_offset = (fpos_t) NULL;
    uint32_t blocks_on_page = PAGE_SIZE / block_size;
    uint8_t in_use;
    Linked_List* empty_blocks_list = init_list();

    do {
        if (page_meta.is_full != 1) {
            for (int i = 0; i < blocks_on_page; ++i) {
                in_use = 1;

                fgetpos(storage_descriptor->storage_file, &empty_offset);
                fread(read_block, block_size, 1, storage_descriptor->storage_file);

                switch (block_type) {
                    case NODE_BLOCK_TYPE:
                        if (read_block->node.in_use == 0) { in_use = 0; }
                        break;
                    case PROP_BLOCK_TYPE:
                        if (read_block->property.in_use == 0) { in_use = 0; }
                        break;
                    case RELATION_BLOCK_TYPE:
                        if (read_block->relation.in_use == 0) { in_use = 0; }
                        break;
                    case KEYS_BLOCK_TYPE:
                        if (read_block->property_key.in_use == 0) { in_use = 0; }
                        break;
                    case LABELS_BLOCK_TYPE:
                        if (read_block->label.in_use == 0) { in_use = 0; }
                        break;
                    default:
                        printf("Weird block type = %d", block_type);
                }

                if (in_use == 0) {
                    Free_Block free_block;
                    free_block.offset = empty_offset;
                    free_block.id = page_meta.page_number * blocks_on_page + i;
                    add_last(empty_blocks_list, &free_block);
                }
            }
        }

        if (page_meta.next_page_offset != (uint32_t) NULL) {
            fseek(storage_descriptor->storage_file, (long) page_meta.next_page_offset, SEEK_SET);
            fread(&page_meta, sizeof(Page_Meta), 1, storage_descriptor->storage_file);
        }

    }
    while (page_meta.next_page_offset != (uint32_t) NULL);

    free(read_block);

    printf("Found %d empty blocks of type %d\n", empty_blocks_list->size, block_type);
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

    (*storage_descriptor)->free_blocks_of_type[PAGES_INDEX] = NULL;
    for (uint8_t block_type = NODE_BLOCK_TYPE; block_type <= COUNTERS_AMOUNT; ++block_type) {
        (*storage_descriptor)->free_blocks_of_type[block_type] = find_empty_blocks(block_type, *storage_descriptor);
    }

    printf("Storage Meta init: mark = %d, first nodes page offset = %d, pages = %d\n",
            (*storage_descriptor)->meta->storage_mark,
            (*storage_descriptor)->meta->first_page_offsets[NODE_BLOCK_TYPE],
            (*storage_descriptor)->meta->pages_by_type[PAGES_INDEX]);

    return 0;
}

uint8_t close_storage(Storage_Descriptor* storage_descriptor) {
    printf("\nClose storage\n");
    int res = fclose(storage_descriptor->storage_file);
    free(storage_descriptor->meta);
    printf("Free lists\n");
    for (int i = NODE_BLOCK_TYPE; i <= INIT_PAGES_COUNT; ++i) {
        if (storage_descriptor->free_blocks_of_type[i] != NULL) {
            free_list(storage_descriptor->free_blocks_of_type[i], 0);
        }
    }
    printf("Free storage\n");
//    free(storage_descriptor);
    return res;
}

uint8_t get_block_by_id(uint32_t id, void* block, uint8_t block_type, Storage_Descriptor* storage_descriptor) {
    uint8_t block_size = get_block_size_by_type(block_type);
    uint32_t blocks_per_page = PAGE_SIZE / block_size;

    uint32_t page_to_find = id / blocks_per_page;

    uint32_t first_page_offset = storage_descriptor->meta->first_page_offsets[block_type];

    Page_Meta page_meta;
    fseek(storage_descriptor->storage_file, (long) first_page_offset, SEEK_SET);
    fread(&page_meta, sizeof(Page_Meta), 1, storage_descriptor->storage_file);

    while (page_meta.page_number != page_to_find && page_meta.next_page_offset != (uint32_t) NULL) {
        fseek(storage_descriptor->storage_file, (long) page_meta.next_page_offset, SEEK_SET);
        fread(&page_meta, sizeof(Page_Meta), 1, storage_descriptor->storage_file);
    }

    uint32_t id_on_page = id % blocks_per_page;
    fseek(storage_descriptor->storage_file, (long) id_on_page * block_size, SEEK_CUR);
    fread(&block, block_size, 1, storage_descriptor->storage_file);

    return 0;
}

uint32_t find_label_id(char* label, uint8_t label_type, Storage_Descriptor* storage_descriptor) {
    if (label_type != KEYS_BLOCK_TYPE && label_type != LABELS_BLOCK_TYPE) {
        printf("Wrong label type = %d\n", label_type);
        return -1;
    }

    uint32_t start_offset = storage_descriptor->meta->first_page_offsets[label_type];

    Page_Meta page_meta;
    fseek(storage_descriptor->storage_file, (long) start_offset, SEEK_SET);
    fread(&page_meta, sizeof(Page_Meta), 1, storage_descriptor->storage_file);

    printf("Labels Page meta (%d): page type = %d, page number = %d, next = %d\n", sizeof(Page_Meta),
            page_meta.page_type,
            page_meta.page_number, page_meta.next_page_offset);

    Label read_label;
    uint32_t label_offset = (uint32_t) NULL;
    uint32_t blocks_on_page = PAGE_SIZE / sizeof(Label);

    do {
        if (page_meta.is_full != 1) {
            for (int i = 0; i < blocks_on_page && label_offset == (uint32_t) NULL; ++i) {
                fread(&read_label, sizeof(Label), 1, storage_descriptor->storage_file);
                if (read_label.in_use == 1 && strcmp(read_label.label, label) == 0) {
                    label_offset = page_meta.page_number * blocks_on_page + i;
                    printf("Found label %s id = %d", label, label_offset);
                }
            }
        }

        if (label_offset == (uint32_t) NULL && page_meta.next_page_offset != (uint32_t) NULL) {
            fseek(storage_descriptor->storage_file, (long) page_meta.next_page_offset, SEEK_SET);
            fread(&page_meta, sizeof(Page_Meta), 1, storage_descriptor->storage_file);
        }
    }
    while (page_meta.next_page_offset != (uint32_t) NULL && label_offset == (uint32_t) NULL);

    return label_offset;
}

uint32_t add_label(char* label_value, uint8_t label_type, Storage_Descriptor* storage_descriptor) {
    if (label_type != KEYS_BLOCK_TYPE && label_type != LABELS_BLOCK_TYPE) {
        printf("Wrong label type = %d\n", label_type);
        return -1;
    }

    Free_Block* free_block = (Free_Block*) storage_descriptor->free_blocks_of_type[label_type]->first->value;
    uint32_t label_id = free_block->id;

    Label label;
    label.in_use = 1;
    strcat_s(label.label, MAX_LABEL_LENGTH / 8, label_value);
    label.records_count = 1;

    fseek(storage_descriptor->storage_file, (long) free_block->offset, SEEK_SET);
    fwrite(&label, sizeof(label), 1, storage_descriptor->storage_file);

    remove_first(storage_descriptor->free_blocks_of_type[label_type]);

    fflush(storage_descriptor->storage_file);

    return label_id;
}

uint32_t add_properties(Linked_List* properties, Storage_Descriptor* storage_descriptor) {
    uint32_t first_prop_id = (uint32_t) NULL;

    Linked_List* free_blocks_list = storage_descriptor->free_blocks_of_type[PROP_BLOCK_TYPE];
    List_Node* current_free_node;
    Free_Block* free_block;

    if (properties->size > 0) {
        current_free_node = free_blocks_list->first;
        free_block = current_free_node->value;

        first_prop_id = free_block->id;

        List_Node* current_prop_node = properties->first;
        Prop_Descriptor* prop_descriptor;

        for (uint32_t i = 0; i < properties->size; ++i) {
            prop_descriptor = current_prop_node->value;

            uint32_t key_id = find_label_id(prop_descriptor->key, KEYS_BLOCK_TYPE, storage_descriptor);
            if (key_id == (uint32_t) NULL) {
                add_label(prop_descriptor->key, KEYS_BLOCK_TYPE, storage_descriptor);
            }
            free_block = current_free_node->value;

            Property property;
            property.key_id = key_id;
            strcat_s(property.value, MAX_LABEL_LENGTH / 8, prop_descriptor->value);
            property.type = prop_descriptor->type;
            property.next_prop_id = ((Free_Block*) current_free_node->next->value)->id;

            fseek(storage_descriptor->storage_file, (long) free_block->offset, SEEK_SET);
            fwrite(&property, sizeof(Property), 1, storage_descriptor->storage_file);

            current_free_node = current_free_node->next;
            remove_first(free_blocks_list);

            current_prop_node = current_prop_node->next;
        }

        fflush(storage_descriptor->storage_file);
    }

    return first_prop_id;
}

uint32_t find_last_relation_id(uint32_t node_id, Storage_Descriptor* storage_descriptor) {
    Node node;
    get_block_by_id(node_id, &node, NODE_BLOCK_TYPE, storage_descriptor);

    uint32_t relation_id = node.first_rel_id;

    if (relation_id != (uint32_t) NULL) {
        uint32_t next_relation_id = (uint32_t) NULL;
        Relation relation;

        uint8_t found = 0;
        do {
            get_block_by_id(relation_id, &relation, RELATION_BLOCK_TYPE, storage_descriptor);
            if (relation.from_id == node_id) { next_relation_id = relation.from_next_rel_id; }
            else if (relation.to_id == node_id) { next_relation_id = relation.to_next_rel_id; }
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

uint8_t add_relations(Linked_List* relations_list, Storage_Descriptor* storage_descriptor) {
    Linked_List* free_rel_blocks_list = storage_descriptor->free_blocks_of_type[RELATION_BLOCK_TYPE];

    uint32_t first_property_id;
    if (relations_list->size > 0) {
        List_Node* current_free_node = free_rel_blocks_list->first;

        List_Node* current_rel_node = relations_list->first;
        Relation_Descriptor* rel_descriptor;

        for (uint32_t i = 0; i < relations_list->size; ++i) {

            rel_descriptor = current_rel_node->value;

            Relation relation;
            relation.in_use = 1;

            uint32_t rel_label_id = find_label_id(rel_descriptor->label, LABELS_BLOCK_TYPE, storage_descriptor);
            if (rel_label_id == (uint32_t) NULL) {
                add_label(rel_descriptor->label, LABELS_BLOCK_TYPE, storage_descriptor);
            }
            relation.label_id = rel_label_id;

            first_property_id = add_properties(rel_descriptor->properties, storage_descriptor);
            relation.first_prop_id = first_property_id;

            relation.from_id = rel_descriptor->first_node_id;
            relation.from_prev_rel_id = find_last_relation_id(rel_descriptor->first_node_id, storage_descriptor);
            relation.from_next_rel_id = (uint32_t) NULL;

            relation.to_id = rel_descriptor->second_node_id;
            relation.to_prev_rel_id = find_last_relation_id(rel_descriptor->second_node_id, storage_descriptor);
            relation.to_next_rel_id = (uint32_t) NULL;

            uint32_t offset = ((Free_Block*) current_free_node->value)->offset;
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
    node.in_use = 1;

    uint32_t label_id;
    char* label;
    if (node_descriptor->labels != NULL) {
        List_Node* current_label_node = node_descriptor->labels->first;

        for (uint32_t i = 0; i < node_descriptor->labels->size; ++i) {
            label = current_label_node->value;

            if (label != NULL) {
                label_id = find_label_id(label, LABELS_BLOCK_TYPE, storage_descriptor);
                if (label_id == (uint32_t) NULL) {
                    label_id = add_label(label, LABELS_BLOCK_TYPE, storage_descriptor);
                }
                node.label_id[i] = label_id;
            }

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

    Free_Block* free_node_block = storage_descriptor->free_blocks_of_type[NODE_BLOCK_TYPE]->first->value;
    fseek(storage_descriptor->storage_file, (long) free_node_block->offset, SEEK_SET);
    fwrite(&node, sizeof(node), 1, storage_descriptor->storage_file);

    fflush(storage_descriptor->storage_file);

    return 0;
}

