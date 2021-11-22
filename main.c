#include <stdio.h>
#include "./storage/storage.h"

int main() {
    printf("Hello, Storage!\n");

    /*printf("Node size = %d\n", sizeof(Node));
    printf("Rel size = %d\n", sizeof(Relation));
    printf("Prop size = %d\n", sizeof(Property));
    printf("Labels size = %d\n", sizeof(Labels));
    printf("Property Value size = %d\n", sizeof(Property_Value));
    printf("Block size = %d\n", sizeof(Block));*/

    Storage_Descriptor* storage_descriptor;

    open_storage("storage.s", &storage_descriptor);

    printf("\nStorage Meta in main: mark = %d, last block id = %d, pages = %d\n",
            storage_descriptor->meta->storage_mark,
            storage_descriptor->meta->last_block_id,
            storage_descriptor->meta->blocks_by_type[PAGES_INDEX]);

    for (int i = 0; i < COUNTERS_AMOUNT; ++i) {
        printf("Counter for type %d = %d\n", i, storage_descriptor->meta->blocks_by_type[i]);
    }

    if (storage_descriptor->free_blocks_list != NULL)
        printf("Free blocks count = %d\n", storage_descriptor->free_blocks_list->size);

    close_storage(storage_descriptor);

    return 0;
}
