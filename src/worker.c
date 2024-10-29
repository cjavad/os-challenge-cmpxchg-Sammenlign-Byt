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
        pool->workers[i].scheduler = scheduler;
        pthread_create(&pool->workers[i].thread, NULL, worker_thread, &pool->workers[i]);
    }

    return pool;
}

void worker_destroy_pool(WorkerPool* pool) {
    scheduler_close(pool->scheduler);

    // Wait for all worker threads to exit.
    for (size_t i = 0; i < pool->size; i++) {
        pthread_join(pool->workers[i].thread, NULL);
    }

    // Clean up the pool.
    free(pool->workers);
    free(pool);
}

void* worker_thread(void* arguments) {
    const WorkerState* worker_state = arguments;

    Scheduler* scheduler = worker_state->scheduler;

    HashDigest hash;
    uint32_t last_job_id = UINT32_MAX;
    struct ScheduledJobs local_sched_jobs = {0};

    while (scheduler->running) {
        uint32_t job_idx;
        uint64_t start;
        uint64_t end;

        if (!scheduler_schedule(scheduler, &local_sched_jobs, &last_job_id, hash, &job_idx, &start, &end)) {
            continue;
        }

        scheduler_job_rc_enter(scheduler, job_idx);

        const uint64_t answer = reverse_sha256_x4(start, end, hash);

        // Task did not find answer for job.
        if (answer == 0) {
            scheduler_job_rc_leave(scheduler, job_idx);
            continue;
        }

        // Notify the scheduler that the job is done.
        scheduler_job_done(scheduler, job_idx, answer);

        // Store cache entry.
        cache_insert_pending(scheduler->cache, hash, answer);

        scheduler_job_rc_leave(scheduler, job_idx);
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