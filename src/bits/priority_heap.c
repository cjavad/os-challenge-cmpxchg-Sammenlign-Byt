#include "priority_heap.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

void priority_heap_swap_indices(
    const _AnyPriorityHeap* heap,
    const uint32_t i,
    const uint32_t j,
    const uint32_t node_size
) {
    _AnyPriorityHeapNode* node_i = vec_get_unsafe(heap, i, node_size);
    _AnyPriorityHeapNode* node_j = vec_get_unsafe(heap, j, node_size);

    uint8_t tmp[node_size];
    memcpy(tmp, node_i, node_size);
    memcpy(node_i, node_j, node_size);
    memcpy(node_j, tmp, node_size);
}

inline uint32_t priority_heap_parent(
    const uint32_t i
) {
    return (i - 1) / 2;
}
inline uint32_t priority_heap_left(
    const uint32_t i
) {
    return 2 * i + 1;
}
inline uint32_t priority_heap_right(
    const uint32_t i
) {
    return 2 * i + 2;
}

void priority_heap_shift_up(
    const _AnyPriorityHeap* heap,
    uint32_t i,
    const uint32_t node_size
) {
    while (i > 0) {
        const uint32_t parent = priority_heap_parent(i);

        const _AnyPriorityHeapNode* current =
            vec_get_unsafe(heap, i, node_size);
        const _AnyPriorityHeapNode* parent_node =
            vec_get_unsafe(heap, parent, node_size);

        if (current->priority <= parent_node->priority) {
            break;
        }

        priority_heap_swap_indices(heap, i, parent, node_size);

        i = parent;
    }
}

void priority_heap_shift_down(
    const _AnyPriorityHeap* heap,
    uint32_t i,
    const uint32_t node_size
) {
    uint32_t max_idx = i;

    while (true) {
        const _AnyPriorityHeapNode* max =
            vec_get_unsafe(heap, max_idx, node_size);

        const uint32_t left_idx = priority_heap_left(i);
        const uint32_t right_idx = priority_heap_right(i);

        const _AnyPriorityHeapNode* left =
            vec_get_unsafe(heap, left_idx, node_size);
        const _AnyPriorityHeapNode* right =
            vec_get_unsafe(heap, right_idx, node_size);

        if (left_idx < heap->len && left->priority > max->priority) {
            max_idx = left_idx;
        }

        max = vec_get_unsafe(heap, max_idx, node_size);

        if (right_idx < heap->len && right->priority > max->priority) {
            max_idx = right_idx;
        }

        if (max_idx != i) {
            priority_heap_swap_indices(heap, i, max_idx, node_size);
            i = max_idx;
            continue;
        }

        break;
    }
}

void _priority_heap_insert(
    _AnyPriorityHeap* heap,
    const void* elem,
    const uint32_t priority,
    const uint32_t node_size,
    const uint32_t elem_size
) {
    PriorityHeapNode(union { uint8_t elem[elem_size]; }) node;
    memset(&node, 0, sizeof(node));
    node.priority = priority;
    memcpy(node.elem.elem, elem, elem_size);
    vec_push_unsafe(heap, &node, node_size);
    priority_heap_shift_up(heap, heap->len - 1, node_size);
}

void _priority_heap_remove(
    _AnyPriorityHeap* heap,
    const uint32_t i,
    const uint32_t node_size
) {
    if (i >= heap->len) {
        return;
    }

    _AnyPriorityHeapNode* current = vec_get_unsafe(heap, i, node_size);
    current->priority = UINT32_MAX;

    priority_heap_shift_up(heap, i, node_size);

    _priority_heap_extract_max(heap, node_size, NULL);
}

void _priority_heap_get_max(
    const _AnyPriorityHeap* heap,
    const uint32_t node_size,
    _AnyPriorityHeapNode** node
) {
    if (heap->len == 0) {
        *node = NULL;
        return;
    }

    *node = vec_get_unsafe(heap, 0, node_size);
}

void _priority_heap_extract_max(
    _AnyPriorityHeap* heap,
    const uint32_t node_size,
    _AnyPriorityHeapNode* node
) {
    if (heap->len == 0) {
        return;
    }

    if (node != NULL) {
        memcpy(node, vec_get_unsafe(heap, 0, node_size), node_size);
    }

    priority_heap_swap_indices(heap, 0, heap->len - 1, node_size);
    vec_pop_unsafe(heap, node_size);
    priority_heap_shift_down(heap, 0, node_size);
}
