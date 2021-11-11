#include <stdio.h>
#include "./storage/storage.h"

int main() {
    printf("Hello, Storage!\n");
    Storage_Descriptor* storage_descriptor;

    open_storage("storage.s", &storage_descriptor);

    printf("\nStorage Meta in main: mark = %d, first nodes page offset = %d, pages = %d\n",
            storage_descriptor->meta->storage_mark,
            storage_descriptor->meta->first_page_offsets[NODE_BLOCK_TYPE],
            storage_descriptor->meta->counters_by_type[PAGES_INDEX]);

    for (int i = 0; i < COUNTERS_AMOUNT; ++i) {
        printf("First page offset for type %d = %d\n", i, storage_descriptor->meta->first_page_offsets[i]);
        printf("Counter for type %d = %d\n", i, storage_descriptor->meta->counters_by_type[i]);
        if (storage_descriptor->free_blocks_of_type[i] != NULL)
            printf("Free blocks of type %d = %d\n", i, storage_descriptor->free_blocks_of_type[i]->size);
    }

    close_storage(storage_descriptor);

    return 0;
}
