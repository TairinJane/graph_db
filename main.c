#include <stdio.h>
#include <malloc.h>
#include "./storage/storage.h"
#include "./storage/data.h"

int main() {
    printf("Hello, Storage!\n");

    /*printf("Node size = %d\n", sizeof(Node));
    printf("Rel size = %d\n", sizeof(Relation));
    printf("Prop size = %d\n", sizeof(Property));
    printf("Labels size = %d\n", sizeof(Labels));
    printf("Property Value size = %d\n", sizeof(String_Value));
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

    Node_Descriptor* node_descriptor = malloc(sizeof(Node_Descriptor));

    printf("\nAdd props\n");

    Linked_List* props = init_list();
    Prop_Descriptor* prop_descriptor;
    for (int i = 0; i < 5; ++i) {
        prop_descriptor = malloc(sizeof(Prop_Descriptor));

        prop_descriptor->value = malloc(7);
        sprintf(prop_descriptor->value, "Value %d", i);

        prop_descriptor->key = malloc(5);
        sprintf(prop_descriptor->key, "Key %d", i);

        prop_descriptor->type = i;

        add_last(props, prop_descriptor);
        printf("Add prop %d\n", i);
    }
    node_descriptor->properties = props;

    printf("\nAdd labels\n");

    Linked_List* labels = init_list();
    char* l[2];
    l[0] = "Node";
    l[1] = "Test";
    for (int i = 0; i < 2; ++i) {
        add_last(labels, l[i]);
        printf("Add label %s\n", l[i]);
    }
    node_descriptor->labels = labels;

    node_descriptor->relations = NULL;

    printf("\nAdd node in main\n");

    uint32_t node_id = add_node(node_descriptor, storage_descriptor);

    printf("\nGet node in main\n");

    get_node(node_id, node_descriptor, storage_descriptor);

    printf("\nPrint node info\n");

    printf("\nProperties\n");
    List_Node* list_node = node_descriptor->properties->first;
    for (int i = 0; i < node_descriptor->properties->size; ++i) {
        prop_descriptor = list_node->value;
        printf("Property: %s = %s, type %d\n", prop_descriptor->key, prop_descriptor->value, prop_descriptor->type);
        list_node = list_node->next;
    }

    printf("\nLabels\n");
    list_node = node_descriptor->labels->first;
    for (int i = 0; i < node_descriptor->labels->size; ++i) {
        printf("Label: %s\n", (char*) list_node->value);
        list_node = list_node->next;
    }

    printf("\nClose everything\n");

//    free_list(props, false);
//    free_list(labels, false);

    close_storage(storage_descriptor);

    return 0;
}
