#include "page_allocator.h"

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#define PAGE_ALLOCATOR_USE_MALLOC 0

struct PageAllocator* page_allocator_create() {
    struct PageAllocator* allocator = calloc(1, sizeof(struct PageAllocator));

    allocator->page_size = sysconf(_SC_PAGESIZE);
    allocator->all_pages = NULL;
    allocator->free_list = NULL;

    return allocator;
}

void* page_allocator_alloc(
    struct PageAllocator* allocator
) {
    // Find a free page.
    struct PageAllocatorHeader* header;

    // Find a free page.
    while ((header = atomic_load(&allocator->free_list)) != NULL &&
           !atomic_compare_exchange_weak(
               &allocator->free_list,
               &header,
               header->next_free
           )) {
    }

    // Create a new page if no free page is found.
    if (header == NULL) {
#if PAGE_ALLOCATOR_USE_MALLOC
        header = malloc(allocator->page_size);
#else
        header = mmap(
            NULL,
            allocator->page_size,
            PROT_READ | PROT_WRITE,
            MAP_ANONYMOUS | MAP_PRIVATE,
            -1,
            0
        );
#endif
        if (header == MAP_FAILED) {
            return NULL;
        }

        header->next_any = NULL;
        header->next_free = NULL;

        // Add to all_pages list
        struct PageAllocatorHeader* old_all_pages =
            atomic_load(&allocator->all_pages);
        do {
            header->next_any = old_all_pages;
        } while (!atomic_compare_exchange_weak(
            &allocator->all_pages,
            &old_all_pages,
            header
        ));
    } else {
        header->next_free = NULL;
    }

    // Give a pointer to the memory.
    return (void*)((uintptr_t)header + sizeof(struct PageAllocatorHeader));
}

void page_allocator_free(
    struct PageAllocator* allocator,
    void* ptr
) {
    struct PageAllocatorHeader* header =
        (struct PageAllocatorHeader*)((uintptr_t)ptr -
                                      sizeof(struct PageAllocatorHeader));

    // Add to free list.
    struct PageAllocatorHeader* old_free_list =
        atomic_load(&allocator->free_list);
    do {
        header->next_free = old_free_list;
    } while (!atomic_compare_exchange_weak(
        &allocator->free_list,
        &old_free_list,
        header
    ));
}

void page_allocator_destroy(
    struct PageAllocator* allocator
) {
    // Free all pages
    struct PageAllocatorHeader* header =
        atomic_exchange(&allocator->all_pages, NULL);
    while (header != NULL) {
        struct PageAllocatorHeader* next = header->next_any;
#if PAGE_ALLOCATOR_USE_MALLOC
        free(header);
#else
        munmap(header, allocator->page_size);
#endif
        header = next;
    }

    free(allocator);
}