#include "worker.h"

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

WorkerPool* worker_create_pool(const size_t size, Scheduler* scheduler) {
    WorkerPool* pool = calloc(1, sizeof(WorkerPool));
    pool->workers = calloc(size, sizeof(WorkerState));
    pool->size = size;
    pool->scheduler = scheduler;

    printf("Creating worker pool with %zu workers\n", size);

    for (size_t i = 0; i < size; i++) {
        pool->workers[i].running = true;
        pool->workers[i].scheduler = scheduler;
        pthread_create(&pool->workers[i].thread, NULL, worker_thread, &pool->workers[i]);
    }

    return pool;
}

void worker_destroy_pool(WorkerPool* pool) {
    scheduler_close(pool->scheduler);

    // Wait for all worker threads to exit.
    for (size_t i = 0; i < pool->size; i++) {
        pool->workers[i].running = false;
        pthread_join(pool->workers[i].thread, NULL);
    }

    // Clean up the pool.
    free(pool->workers);
    free(pool);
}

void* worker_thread(void* arguments) {
    const WorkerState* worker_state = arguments;

    const Scheduler* scheduler = worker_state->scheduler;

    while (worker_state->running) {

        Job* job = scheduler->next_job;

        while (job != NULL) {
            if (!job->done || job->block_idx < job->block_count) {
                break;
            }

            job = job->next;
        }

        if (job == NULL) {
            // todo yield pls
            continue;
        }

        const uint64_t block_idx = atomic_fetch_add(&job->block_idx, 1);

        if (block_idx >= job->block_count) {
            continue;
        }

        const uint64_t start = job->start + block_idx * SCHEDULER_BLOCK_SIZE;
        const uint64_t end = MIN(start + SCHEDULER_BLOCK_SIZE, job->end);

        const uint64_t answer = reverse_sha256_x4(start, end, job->hash);

        // Task did not find answer for job.
        if (answer == 0) {
            continue;
        }

        job->answer = answer;
        job->done = 1;

        // Notify the scheduler that the job is done.
        scheduler_job_done(worker_state->scheduler, job);
    }

    return NULL;
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