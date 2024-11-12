#include "worker_pool.h"

#include "../scheduler/scheduler.h"

#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
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

    fprintf(stderr, "Creating worker pool with %zu workers\n", size);

    const int core_count = worker_pool_get_concurrency();
    cpu_set_t cpuset;

    for (size_t i = 0; i < size; i++) {
        CPU_ZERO(&cpuset);
        const int affinity = i % core_count;
        CPU_SET(affinity, &cpuset);

        pthread_attr_t attr;
        struct sched_param param;
        param.sched_priority = 20;

        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
        pthread_attr_setschedparam(&attr, &param);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset);

        pool->workers[i].scheduler = scheduler;

        pthread_create(
            &pool->workers[i].thread,
            &attr,
            worker_thread,
            scheduler
        );

        pthread_attr_destroy(&attr);
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

int worker_pool_get_concurrency() { return (int)sysconf(_SC_NPROCESSORS_ONLN); }

void worker_set_nice(const int nice) {
    if (nice < -20 || nice > 19) {
        fprintf(stderr, "Invalid nice value %d\n", nice);
        return;
    }

    if (setpriority(PRIO_PROCESS, 0, nice) != 0) {
        fprintf(stderr, "Failed to set nice value %d\n", nice);
    }
}