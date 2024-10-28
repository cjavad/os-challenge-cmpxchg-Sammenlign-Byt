#pragma once

#include "concat.h"
#include "vec.h"
#include <stdint.h>

#define PriorityHeapNode(T)                                                                                            \
    struct {                                                                                                           \
        uint32_t priority;                                                                                             \
        T elem;                                                                                                        \
    }

#define PriorityHeap(T) Vec(PriorityHeapNode(T))

typedef PriorityHeap(void*) _AnyPriorityHeap;
typedef PriorityHeapNode(void*) _AnyPriorityHeapNode;

#define __swap(a, b, c)                                                                                                \
    {                                                                                                                  \
        typeof(a) __CONCAT(____phq_tmp, c) = a;                                                                        \
        a = b;                                                                                                         \
        b = __CONCAT(____phq_tmp, c);                                                                                  \
    }

#define priority_heap_init(T, cap)    vec_init(T, cap)
#define priority_heap_destroy(heap)   vec_destroy(heap)
#define priority_heap_node_size(heap) (sizeof((heap)->data[0]))
#define priority_heap_elem_size(heap) (sizeof((heap)->data[0].elem))
#define priority_heap_node_type(heap) typeof((heap)->data[0])

void _priority_heap_insert(
    _AnyPriorityHeap* heap, const void* elem, uint32_t priority, uint32_t node_size, uint32_t elem_size
);

void _priority_heap_remove(_AnyPriorityHeap* heap, uint32_t i, uint32_t node_size, uint32_t elem_size);

_AnyPriorityHeapNode* _priority_heap_get_min(const _AnyPriorityHeap* heap, uint32_t node_size);
void _priority_heap_pop_min(_AnyPriorityHeap* heap, uint32_t node_size);

_AnyPriorityHeapNode* _priority_heap_get_max(const _AnyPriorityHeap* heap, uint32_t node_size);
void _priority_heap_pop_max(_AnyPriorityHeap* heap, uint32_t node_size);

#define priority_heap_insert(heap, elem, priority)                                                                     \
    _priority_heap_insert(                                                                                             \
        (_AnyPriorityHeap*)heap, elem, priority, priority_heap_node_size(heap), priority_heap_elem_size(heap)          \
    )

#define priority_heap_remove(heap, i)                                                                                  \
    _priority_heap_remove((_AnyPriorityHeap*)heap, i, priority_heap_node_size(heap), priority_heap_elem_size(heap))

#define priority_heap_get_min(heap)                                                                                    \
    ({                                                                                                                 \
        (priority_heap_node_type(heap)*)                                                                                \
            _priority_heap_get_min((_AnyPriorityHeap*)heap, priority_heap_node_size(heap));                            \
    })
#define priority_heap_pop_min(heap) _priority_heap_pop_min((_AnyPriorityHeap*)heap, priority_heap_node_size(heap))

#define priority_heap_get_max(heap)                                                                                    \
    ({                                                                                                                 \
        (priority_heap_node_type(heap)*)                                                                                \
            _priority_heap_get_max((_AnyPriorityHeap*)heap, priority_heap_node_size(heap));                            \
    })
#define priority_heap_pop_max(heap) _priority_heap_pop_max((_AnyPriorityHeap*)heap, priority_heap_node_size(heap))

#define priority_heap_copy(heap, out) vec_copy(heap, out)