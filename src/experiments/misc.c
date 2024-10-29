#include "misc.h"
#include "../bits/priority_heap.h"
#include "../bits/spin.h"

#include <assert.h>
#include <pthread.h>
#include <stdio.h>

void* writer(void* arg) {
    struct SRWLock* lock = arg;

    spin_rwlock_wrlock(lock);

    spin_rwlock_wrunlock(lock);

    return NULL;
}

void* reader(void* arg) {
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

int misc_main() {
    test_priority_heap();
    test_spinlock();

    return 0;
};
