#include "misc.h"
#include "../bits/priority_heap.h"

#include <stdio.h>

int misc_main() {
    PriorityHeap(uint32_t) heap;
    priority_heap_init(&heap, 8);

    uint32_t elem = 1;

    priority_heap_insert(&heap, &elem, 2);

    elem = 10;

    priority_heap_insert(&heap, &elem, 1);
    priority_heap_insert(&heap, &elem, 1);
    priority_heap_insert(&heap, &elem, 1);
    priority_heap_insert(&heap, &elem, 1);
    priority_heap_insert(&heap, &elem, 3);
    priority_heap_insert(&heap, &elem, 1);
    priority_heap_insert(&heap, &elem, 1);
    priority_heap_insert(&heap, &elem, 1);

    elem = 20;

    priority_heap_insert(&heap, &elem, 0);

    for (uint32_t i = 0; i < heap.len; i++) {
        const PriorityHeapNode(uint32_t)* node = (void*)priority_heap_get_max(&heap);
        printf("Priority: %u, Elem: %u\n", node->priority, node->elem);
        priority_heap_pop_max(&heap);
    }

    priority_heap_destroy(&heap);

    return 0;
};
