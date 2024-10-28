#include "misc.h"
#include "../bits/priority_heap.h"
#include "../bits/spin.h"

#include <pthread.h>
#include <stdio.h>

void* writer(void* arg) {
    struct SRWLock* lock = arg;

    printf("Writer waiting\n");

    spin_rwlock_wrlock(lock);

    printf("Writer locked\n");

    spin_rwlock_wrunlock(lock);

    return NULL;
}

void* reader(void* arg) {
    struct SRWLock* lock = arg;

    printf("Reader waiting\n");

    spin_rwlock_rdlock(lock);

    printf("Reader locked\n");

    spin_rwlock_rdunlock(lock);

    return NULL;
}

int misc_main() {
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

    printf("All threads joined\n");
    free(lock);

    return 0;
    ;
};
