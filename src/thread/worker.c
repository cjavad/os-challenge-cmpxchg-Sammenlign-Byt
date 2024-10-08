#include "worker.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

WorkerPool* worker_create_pool(const size_t size, Scheduler* scheduler) {
    WorkerPool* pool = calloc(1, sizeof(WorkerPool));
    pool->workers = calloc(size, sizeof(WorkerState));
    pool->size = size;
    pool->scheduler = scheduler;

    for (size_t i = 0; i < size; i++) {
        pool->workers[i].scheduler = scheduler;
        pthread_create(&pool->workers[i].thread, NULL, worker_thread, &pool->workers[i]);
    }

    return pool;
}

void worker_destroy_pool(WorkerPool* pool) {
    scheduler_empty(pool->scheduler);

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

    while (1) {
        Task task;

        if (!scheduler_schedule(worker_state->scheduler, &task)) {
            continue;
        }

        ProtocolResponse response;

        response.answer = reverse_hash(task.start, task.end, task.hash);

        // Just send the response back to the client.
        protocol_response_to_be(&response);
        send(task.data.fd, &response, PROTOCOL_RES_SIZE, 0);
        close(task.data.fd);
    }

    return NULL;
}
