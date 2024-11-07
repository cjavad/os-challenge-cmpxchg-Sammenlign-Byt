#include "worker.h"

#include "scheduler/scheduler.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct WorkerPool* worker_create_pool(
    const size_t size,
    struct SchedulerBase* scheduler,
    void* (*worker_thread)(void*)
) {
    struct WorkerPool* pool = calloc(1, sizeof(struct WorkerPool));
    pool->workers = calloc(size, sizeof(struct WorkerState));
    pool->size = size;
    pool->scheduler = scheduler;

    printf("Creating worker pool with %zu workers\n", size);

    for (size_t i = 0; i < size; i++) {
        pool->workers[i].scheduler = scheduler;
        pthread_create(
            &pool->workers[i].thread,
            NULL,
            worker_thread,
            scheduler
        );
    }

    return pool;
}

void worker_destroy_pool(
    struct WorkerPool* pool
) {
    scheduler_base_close(pool->scheduler);

    // Wait for all worker threads to exit.
    for (size_t i = 0; i < pool->size; i++) {
        pthread_join(pool->workers[i].thread, NULL);
    }

    // Clean up the pool.
    free(pool->workers);
    free(pool);
}

int cpu_core_count() {
    int64_t count = sysconf(_SC_NPROCESSORS_ONLN);

#ifdef RELEASE
    if (count < 16) {
        return 16;
    }
#endif

    return count;
}