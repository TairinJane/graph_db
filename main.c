#include <stdio.h>
#include "./storage/storage.h"

int main() {
    printf("Hello, Storage!\n");
    Storage_Descriptor* storage_descriptor;

    open_storage("storage.s", &storage_descriptor);

    printf("Storage Meta in main: mark = %d, first nodes page offset = %d, pages = %d\n", storage_descriptor->meta->storage_mark,
            storage_descriptor->meta->first_page_offsets.nodes, storage_descriptor->meta->counters.pages_count);

    find_empty_block(NODE_BLOCK_TYPE, storage_descriptor);

    close_storage(storage_descriptor);

    return 0;
}
