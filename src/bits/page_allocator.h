#pragma once

#include <stdint.h>

// Slow linked list thread-safe page allocator.
// We maintain two linked lists
// one for allocation (free_list)
// one for cleanup    (all_pages)

struct PageAllocator {
    struct PageAllocatorHeader* all_pages;
    struct PageAllocatorHeader* free_list;
    uint64_t page_size;
};

struct PageAllocatorHeader {
    struct PageAllocatorHeader* next_any;
    struct PageAllocatorHeader* next_free;
};

// The writable space of the page.
#define page_allocator_page_size(allocator)                                    \
    ((allocator)->page_size - sizeof(struct PageAllocatorHeader))

struct PageAllocator* page_allocator_create();
void* page_allocator_alloc(struct PageAllocator* allocator);
void page_allocator_free(struct PageAllocator* allocator, void* ptr);
void page_allocator_destroy(struct PageAllocator* allocator);