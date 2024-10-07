#include "worker.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

WorkerPool* worker_create_pool(const size_t size) {
    WorkerPool* pool = calloc(1, sizeof(WorkerPool));
    pool->workers = calloc(size, sizeof(WorkerState));
    pool->size = size;
    // Should be size of ulimit + backlog + some buffer.
    pool->pending = queue_create(8096);

    for (size_t i = 0; i < size; i++) {
        pool->workers[i].pending = pool->pending;
        pthread_create(&pool->workers[i].thread, NULL, worker_thread, &pool->workers[i]);
    }

    return pool;
}

void worker_destroy_pool(WorkerPool* pool) {
    // Fill the queue with NULLs to signal the worker threads to exit.
    for (size_t i = 0; i < pool->size; i++) {
        if (queue_full(pool->pending)) {
            queue_pop(pool->pending);
        }

        queue_push(pool->pending, NULL);
    }

    // Wait for all worker threads to exit.
    for (size_t i = 0; i < pool->size; i++) {
        pthread_join(pool->workers[i].thread, NULL);
    }

    // Clean up the queue and the pool.
    queue_destroy(pool->pending);
    free(pool->workers);
    free(pool);
}

void* worker_thread(void* arguments) {
    const WorkerState* worker_state = arguments;

    while (1) {
        TaskState* state = queue_pop(worker_state->pending);

        if (state == NULL) {
            break;
        }

        state->response.answer = reverse_hash(state->request.start, state->request.end, state->request.hash);

        // Just send the response back to the client.
        protocol_response_to_be(&state->response);
        send(state->data.fd, &state->response, PROTOCOL_RES_SIZE, 0);
        close(state->data.fd);
        scheduler_destroy_task(state);
    }

    return NULL;
}

int worker_pool_submit(const WorkerPool* pool, TaskState* state) {
    queue_push(pool->pending, state);
    return 0;
}