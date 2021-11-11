//
// Created by Anna.Kozhemyako on 28.10.2021.
//

#include <stdlib.h>

#include "storage.h"
#include "util.h"

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
    uint8_t null = (uint8_t) NULL;
    fwrite(&null, 1, 1, file);

    printf("Pages created\n");

    meta->counters_by_type[PAGES_INDEX] = INIT_PAGES_COUNT;
    meta->counters_by_type[NODE_BLOCK_TYPE] = 0;
    meta->counters_by_type[RELATION_BLOCK_TYPE] = 0;
    meta->counters_by_type[PROP_BLOCK_TYPE] = 0;
    meta->counters_by_type[LABELS_BLOCK_TYPE] = 0;
    meta->counters_by_type[KEYS_BLOCK_TYPE] = 0;

    printf("Meta init\n");

    fseek(file, 0, SEEK_SET);
    size_t count = fwrite(meta, sizeof(Storage_Meta), 1, file);
    if (count != 1) {
        printf("Failed meta write\n");
        return 1;
    }

    return 0;
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
    for (uint8_t block_type = NODE_BLOCK_TYPE; block_type <= INIT_PAGES_COUNT; ++block_type) {
        (*storage_descriptor)->free_blocks_of_type[block_type] = find_empty_blocks(block_type, *storage_descriptor);
    }

    printf("Storage Meta init: mark = %d, first nodes page offset = %d, pages = %d\n",
            (*storage_descriptor)->meta->storage_mark,
            (*storage_descriptor)->meta->first_page_offsets[NODE_BLOCK_TYPE],
            (*storage_descriptor)->meta->counters_by_type[PAGES_INDEX]);

//    fclose(storage_file);
    return 0;
}

uint8_t close_storage(Storage_Descriptor* storage_descriptor) {
    printf("\nClose storage\n");
    int res = fclose(storage_descriptor->storage_file);
    free(storage_descriptor->meta);
    printf("Free lists\n");
    for (int i = NODE_BLOCK_TYPE; i <= INIT_PAGES_COUNT; ++i) {
        if (storage_descriptor->free_blocks_of_type[i] != NULL)
            free_list(storage_descriptor->free_blocks_of_type[i], 0);
    }
    printf("Free storage\n");
//    free(storage_descriptor);
    return res;
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
            storage_descriptor->meta->counters_by_type[PAGES_INDEX]);

    uint32_t start_offset = storage_descriptor->meta->first_page_offsets[block_type];
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
            return NULL;
    }

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
                        if (read_block->node.in_use == 0) in_use = 0;
                        break;
                    case PROP_BLOCK_TYPE:
                        if (read_block->property.in_use == 0) in_use = 0;
                        break;
                    case RELATION_BLOCK_TYPE:
                        if (read_block->relation.in_use == 0) in_use = 0;
                        break;
                    case KEYS_BLOCK_TYPE:
                        if (read_block->property_key.in_use == 0) in_use = 0;
                        break;
                    case LABELS_BLOCK_TYPE:
                        if (read_block->label.in_use == 0) in_use = 0;
                        break;
                    default:
                        printf("Weird block type = %d", block_type);
                }

                if (in_use == 0) {
                    add_last(empty_blocks_list, &empty_offset);
                }
            }
        }

        if (page_meta.next_page_offset != (uint32_t) NULL) {
            fseek(storage_descriptor->storage_file, (long) page_meta.next_page_offset, SEEK_SET);
            fread(&page_meta, sizeof(Page_Meta), 1, storage_descriptor->storage_file);
        }

    } while (page_meta.next_page_offset != (uint32_t) NULL);

    free(read_block);

    printf("Found %d empty blocks of type %d\n", empty_blocks_list->size, block_type);
    return empty_blocks_list;
}

uint8_t write_block(Block* block, uint8_t block_type, Storage_Meta* storage_meta) {

}

