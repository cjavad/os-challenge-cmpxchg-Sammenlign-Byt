#include "priority_heap.h"
#include <string.h>

void priority_heap_swap_indices(
    const _AnyPriorityHeap* heap, const uint32_t i, const uint32_t j, const uint32_t node_size
) {
    _AnyPriorityHeapNode* node_i = vec_get_unsafe(heap, i, node_size);
    _AnyPriorityHeapNode* node_j = vec_get_unsafe(heap, j, node_size);

    uint8_t tmp[node_size];
    memcpy(tmp, node_i, node_size);
    memcpy(node_i, node_j, node_size);
    memcpy(node_j, tmp, node_size);
}

void priority_heap_heapify_down(
    _AnyPriorityHeap* heap, const uint32_t i, const uint32_t node_size, const uint32_t elem_size
) {
    const uint32_t left = 2 * i + 1;
    const uint32_t right = 2 * i + 2;
    uint32_t smallest = i;

    // Check if left child exists and if its priority is smaller than the current node
    if (left < heap->len) {
        const _AnyPriorityHeapNode* left_node = vec_get_unsafe(heap, left, node_size);
        const _AnyPriorityHeapNode* node = vec_get_unsafe(heap, i, node_size);
        if (left_node->priority < node->priority) {
            smallest = left;
        }
    }

    // Check if right child exists and if its priority is smaller than the smallest node found
    if (right < heap->len) {
        const _AnyPriorityHeapNode* right_node = vec_get_unsafe(heap, right, node_size);
        const _AnyPriorityHeapNode* smallest_node = vec_get_unsafe(heap, smallest, node_size);
        if (right_node->priority < smallest_node->priority) {
            smallest = right;
        }
    }

    // If the smallest node is not the current node, swap and continue heapifying down
    if (smallest != i) {
        priority_heap_swap_indices(heap, i, smallest, node_size);
        priority_heap_heapify_down(heap, smallest, node_size, elem_size);
    }
}

void priority_heap_heapify_up(_AnyPriorityHeap* heap, uint32_t i, const uint32_t node_size, const uint32_t elem_size) {
    // Ensure i > 0 before calculating the parent
    if (i <= 0)
        return;

    const uint32_t parent = (i - 1) / 2;
    const _AnyPriorityHeapNode* parent_node = vec_get_unsafe(heap, parent, node_size);
    const _AnyPriorityHeapNode* node = vec_get_unsafe(heap, i, node_size);

    // Compare node with its parent and swap if necessary
    if (parent_node->priority <= node->priority)
        return;

    priority_heap_swap_indices(heap, i, parent, node_size);
    priority_heap_heapify_up(heap, parent, node_size, elem_size);
}

void _priority_heap_insert(
    _AnyPriorityHeap* heap, const void* elem, const uint32_t priority, const uint32_t node_size,
    const uint32_t elem_size
) {
    PriorityHeapNode(struct { uint8_t data[elem_size]; }) node;
    memset(&node, 0, node_size);
    node.priority = priority;

    // Copy the element data to the new node
    memcpy(node.elem.data, elem, elem_size);
    vec_push_unsafe(heap, &node, node_size);

    // Heapify up to maintain heap property after insertion
    priority_heap_heapify_up(heap, heap->len - 1, node_size, elem_size);
}

void _priority_heap_remove(
    _AnyPriorityHeap* heap, const uint32_t i, const uint32_t node_size, const uint32_t elem_size
) {
    if (heap->len == 0) {
        return; // Handle empty heap case
    }

    // Replace the node with the last node
    if (i < heap->len - 1) {
        vec_set_unsafe(heap, i, vec_pop_unsafe(heap, node_size), node_size);
    } else {
        vec_pop_unsafe(heap, node_size); // Remove last element directly if it's the target
    }

    if (i < heap->len) {
        // Check whether to heapify up or down
        const _AnyPriorityHeapNode* node = vec_get_unsafe(heap, i, node_size);
        const _AnyPriorityHeapNode* parent_node = vec_get_unsafe(heap, (i - 1) / 2, node_size);

        if (i > 0 && node->priority < parent_node->priority) {
            priority_heap_heapify_up(heap, i, node_size, elem_size);
        } else {
            priority_heap_heapify_down(heap, i, node_size, elem_size);
        }
    }
}

_AnyPriorityHeapNode* _priority_heap_get_min(const _AnyPriorityHeap* heap, const uint32_t node_size) {
    if (heap->len == 0) {
        return NULL; // Handle empty heap case
    }
    return vec_get_unsafe(heap, 0, node_size); // Min is always at index 0
}
void _priority_heap_pop_min(_AnyPriorityHeap* heap, const uint32_t node_size) {
    if (heap->len == 0) {
        return; // Handle empty heap case
    }

    // Replace the root with the last element
    vec_set_unsafe(heap, 0, vec_pop_unsafe(heap, node_size), node_size);

    // Heapify down to maintain heap property
    if (heap->len > 0) {
        priority_heap_heapify_down(heap, 0, node_size, node_size);
    }
}

_AnyPriorityHeapNode* _priority_heap_get_max(const _AnyPriorityHeap* heap, const uint32_t node_size) {
    if (heap->len == 0) {
        return NULL; // Handle empty heap case
    }
    return vec_get_unsafe(heap, heap->len - 1, node_size); // Max is the last node in the heap
}
void _priority_heap_pop_max(_AnyPriorityHeap* heap, const uint32_t node_size) {
    if (heap->len == 0) {
        return; // Handle empty heap case
    }

    // Remove the last element directly
    vec_pop_unsafe(heap, node_size);
}