#include "worker.h"

#include <sched.h>
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

    while (worker_state->running) {
        Task task;

        if (!scheduler_schedule(worker_state->scheduler, &task)) {
            continue;
        }

        ProtocolResponse response;

        response.answer = reverse_hash_x4(task.start, task.end, task.hash);

        // Task did not find answer for job.
        if (response.answer == 0) {
            continue;
        }

        // Task did find answer for job.
        if (!scheduler_terminate(worker_state->scheduler, task.job_id)) {
        }

        // Notify the scheduler that the job is done.
        scheduler_job_done(worker_state->scheduler, &task, &response);
    }

    return NULL;
}

int cpu_affinity_count() {
    cpu_set_t cs;
    CPU_ZERO(&cs);
    sched_getaffinity(0, sizeof(cs), &cs);
    return CPU_COUNT(&cs);
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