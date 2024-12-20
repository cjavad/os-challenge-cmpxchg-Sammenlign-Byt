#include "misc.h"
#include "../bits/page_allocator.h"
#include "../bits/priority_heap.h"
#include "../bits/spin.h"
#include "../sha256/sha256.h"
#include "../sha256/x1/sha256x1.h"
#include "../sha256/x4/sha256x4.h"
#include "../sha256/x4x2/sha256x4x2.h"
#include "../sha256/x8/sha256x8.h"

#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

void* writer(
    void* arg
) {
    struct SRWLock* lock = arg;

    spin_rwlock_wrlock(lock);

    spin_rwlock_wrunlock(lock);

    return NULL;
}

void* reader(
    void* arg
) {
    struct SRWLock* lock = arg;

    spin_rwlock_rdlock(lock);

    spin_rwlock_rdunlock(lock);

    return NULL;
}

void test_spinlock() {
    struct SRWLock* lock = calloc(1, sizeof(struct SRWLock));
    spin_rwlock_init(lock);

    const uint32_t THREADS = 100;

    pthread_t threads[THREADS];

    for (uint32_t i = 0; i < THREADS; i++) {
        if (i % 10 == 0) {
            pthread_create(&threads[i], NULL, writer, lock);
        } else {
            pthread_create(&threads[i], NULL, reader, lock);
        }
    }

    for (uint32_t i = 0; i < THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    free(lock);

    assert(true);
}

void test_priority_heap() {
    uint64_t val = 0;
    PriorityHeapNode(uint64_t) node = {0};

    PriorityHeap(uint64_t) heap;
    priority_heap_init(&heap, 1);

    val++; // 1
    priority_heap_insert(&heap, &val, 1);

    val++; // 2
    priority_heap_insert(&heap, &val, 2);

    val++; // 3
    priority_heap_insert(&heap, &val, 3);

    val++; // 4
    priority_heap_insert(&heap, &val, 0);

    val++; // 5
    priority_heap_insert(&heap, &val, 4);

    val++; // 6
    priority_heap_insert(&heap, &val, 12);

    assert(heap.len == 6);

    priority_heap_extract_max(&heap, (void*)&node);
    assert(node.elem == 6);

    priority_heap_extract_max(&heap, (void*)&node);
    assert(node.elem == 5);

    priority_heap_extract_max(&heap, (void*)&node);
    assert(node.elem == 3);

    priority_heap_extract_max(&heap, (void*)&node);
    assert(node.elem == 2);

    priority_heap_extract_max(&heap, (void*)&node);
    assert(node.elem == 1);

    priority_heap_extract_max(&heap, (void*)&node);
    assert(node.elem == 4);

    priority_heap_extract_max(&heap, (void*)&node);

    priority_heap_destroy(&heap);
}

#define MISC_PAGE_ALLOCATOR_TEST_THREAD_COUNT 100
#define MISC_PAGE_ALLOCATOR_TEST_ITERATIONS   1000
#define MISC_PAGE_ALLOCATOR_TEST_FREE_EVERY   100

// Spam allocation and deallocation.
void* page_allocator_thread(
    void* arg
) {
    struct PageAllocator* allocator = arg;

    for (uint64_t i = 0; i < MISC_PAGE_ALLOCATOR_TEST_ITERATIONS; i++) {
        void* ptr = page_allocator_alloc(allocator);

        // Only free every other page.
        if (i % MISC_PAGE_ALLOCATOR_TEST_FREE_EVERY == 0) {
            page_allocator_free(allocator, ptr);
        }
    }

    return NULL;
}

void test_page_allocator() {
    struct PageAllocator* allocator = page_allocator_create();

    void* ptr1 = page_allocator_alloc(allocator);
    void* ptr2 = page_allocator_alloc(allocator);
    void* ptr3 = page_allocator_alloc(allocator);

    const uint64_t header_size = sizeof(struct PageAllocatorHeader);

    const struct PageAllocatorHeader* ptr1_header = ptr1 - header_size;
    const struct PageAllocatorHeader* ptr2_header = ptr2 - header_size;
    const struct PageAllocatorHeader* ptr3_header = ptr3 - header_size;

    assert(ptr1_header->next_any == NULL);
    assert(ptr2_header->next_any == ptr1_header);
    assert(ptr3_header->next_any == ptr2_header);

    page_allocator_free(allocator, ptr1);
    page_allocator_free(allocator, ptr2);

    assert(allocator->free_list == ptr2_header);
    assert(allocator->free_list->next_free == ptr1_header);

    assert(page_allocator_alloc(allocator) == ptr2);

    page_allocator_destroy(allocator);

    allocator = page_allocator_create();

    pthread_t threads[MISC_PAGE_ALLOCATOR_TEST_THREAD_COUNT];

    for (uint64_t i = 0; i < MISC_PAGE_ALLOCATOR_TEST_THREAD_COUNT; i++) {
        pthread_create(&threads[i], NULL, page_allocator_thread, allocator);
    }

    for (uint64_t i = 0; i < MISC_PAGE_ALLOCATOR_TEST_THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }

    // Count amount of pages allocated.
    uint64_t page_count = 0;
    uint64_t free_count = 0;

    struct PageAllocatorHeader* header = allocator->all_pages;

    while (header != NULL) {
        page_count++;
        assert(header->next_any != header);
        header = header->next_any;
    }

    header = allocator->free_list;

    while (header != NULL) {
        free_count++;
        assert(header->next_free != header);
        header = header->next_free;
    }

    page_allocator_destroy(allocator);

    assert(
        page_count == MISC_PAGE_ALLOCATOR_TEST_THREAD_COUNT *
                          (MISC_PAGE_ALLOCATOR_TEST_ITERATIONS -
                           (MISC_PAGE_ALLOCATOR_TEST_ITERATIONS /
                            MISC_PAGE_ALLOCATOR_TEST_FREE_EVERY))
    );

    assert(free_count == 0);

    // Leak detector finds additional errors.

    assert(true);
}

bool test_sha256_compare(
    const HashDigest a,
    const HashDigest b
) {
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        if (a[i] != b[i]) {
            return false;
        }
    }

    return true;
}

void test_all_sha256_results() {
    HashDigest reference;

    const uint8_t data[SHA256_INPUT_LENGTH * 8] = {0};

    sha256_custom(reference, data);

    uint8_t hashes[SHA256_DIGEST_LENGTH * 8] = {0};

    sha256_optim(hashes, data);
    assert(test_sha256_compare(reference, hashes));

    sha256_fused(hashes, data);
    assert(test_sha256_compare(reference, hashes));

    sha256_fullyfused(hashes, data);
    assert(test_sha256_compare(reference, hashes));

    sha256x4_optim(hashes, data);
    assert(test_sha256_compare(reference, hashes));
    assert(test_sha256_compare(reference, &hashes[SHA256_DIGEST_LENGTH * 1]));
    assert(test_sha256_compare(reference, &hashes[SHA256_DIGEST_LENGTH * 2]));
    assert(test_sha256_compare(reference, &hashes[SHA256_DIGEST_LENGTH * 3]));

    sha256x4_cyclic(hashes, data);
    assert(test_sha256_compare(reference, hashes));
    assert(test_sha256_compare(reference, &hashes[SHA256_DIGEST_LENGTH * 1]));
    assert(test_sha256_compare(reference, &hashes[SHA256_DIGEST_LENGTH * 2]));
    assert(test_sha256_compare(reference, &hashes[SHA256_DIGEST_LENGTH * 3]));

    sha256x4_fused(hashes, data);
    assert(test_sha256_compare(reference, hashes));
    assert(test_sha256_compare(reference, &hashes[SHA256_DIGEST_LENGTH * 1]));
    assert(test_sha256_compare(reference, &hashes[SHA256_DIGEST_LENGTH * 2]));
    assert(test_sha256_compare(reference, &hashes[SHA256_DIGEST_LENGTH * 3]));

    sha256x4_fullyfused(hashes, data);
    assert(test_sha256_compare(reference, hashes));
    assert(test_sha256_compare(reference, &hashes[SHA256_DIGEST_LENGTH * 1]));
    assert(test_sha256_compare(reference, &hashes[SHA256_DIGEST_LENGTH * 2]));
    assert(test_sha256_compare(reference, &hashes[SHA256_DIGEST_LENGTH * 3]));

    sha256x4_fullyfused_asm(hashes, data);
    assert(test_sha256_compare(reference, hashes));
    assert(test_sha256_compare(reference, &hashes[SHA256_DIGEST_LENGTH * 1]));
    assert(test_sha256_compare(reference, &hashes[SHA256_DIGEST_LENGTH * 2]));
    assert(test_sha256_compare(reference, &hashes[SHA256_DIGEST_LENGTH * 3]));

    sha256x4x2_optim(hashes, data);
    assert(test_sha256_compare(reference, hashes));
    assert(test_sha256_compare(reference, &hashes[SHA256_DIGEST_LENGTH * 1]));
    assert(test_sha256_compare(reference, &hashes[SHA256_DIGEST_LENGTH * 2]));
    assert(test_sha256_compare(reference, &hashes[SHA256_DIGEST_LENGTH * 3]));
    assert(test_sha256_compare(reference, &hashes[SHA256_DIGEST_LENGTH * 4]));
    assert(test_sha256_compare(reference, &hashes[SHA256_DIGEST_LENGTH * 5]));
    assert(test_sha256_compare(reference, &hashes[SHA256_DIGEST_LENGTH * 6]));
    assert(test_sha256_compare(reference, &hashes[SHA256_DIGEST_LENGTH * 7]));

    sha256x4x2_fused(hashes, data);
    assert(test_sha256_compare(reference, hashes));
    assert(test_sha256_compare(reference, &hashes[SHA256_DIGEST_LENGTH * 1]));
    assert(test_sha256_compare(reference, &hashes[SHA256_DIGEST_LENGTH * 2]));
    assert(test_sha256_compare(reference, &hashes[SHA256_DIGEST_LENGTH * 3]));
    assert(test_sha256_compare(reference, &hashes[SHA256_DIGEST_LENGTH * 4]));
    assert(test_sha256_compare(reference, &hashes[SHA256_DIGEST_LENGTH * 5]));
    assert(test_sha256_compare(reference, &hashes[SHA256_DIGEST_LENGTH * 6]));
    assert(test_sha256_compare(reference, &hashes[SHA256_DIGEST_LENGTH * 7]));

    /*sha256x8_optim(hashes, data);
    assert(test_sha256_compare(reference, hashes));
    assert(test_sha256_compare(reference, &hashes[SHA256_DIGEST_LENGTH * 1]));
    assert(test_sha256_compare(reference, &hashes[SHA256_DIGEST_LENGTH * 2]));
    assert(test_sha256_compare(reference, &hashes[SHA256_DIGEST_LENGTH * 3]));
    assert(test_sha256_compare(reference, &hashes[SHA256_DIGEST_LENGTH * 4]));
    assert(test_sha256_compare(reference, &hashes[SHA256_DIGEST_LENGTH * 5]));
    assert(test_sha256_compare(reference, &hashes[SHA256_DIGEST_LENGTH * 6]));
    assert(test_sha256_compare(reference, &hashes[SHA256_DIGEST_LENGTH * 7]));
    */
}

int misc_main() {
    HashDigest target;
    uint64_t actual = 1;
    sha256_custom(target, (uint8_t*)&actual);

    const uint64_t res = reverse_sha256x4_fullyfused_asm_2(0, 3, target);

    printf("res: %lu\n", res);

    // test_all_sha256_results();
    // test_priority_heap();
    // test_spinlock();
    // test_page_allocator();
    // misc_pthread_waker_vs_pure_futex();
    return 0;
}

;