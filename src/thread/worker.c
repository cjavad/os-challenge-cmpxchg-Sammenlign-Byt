#include "worker.h"

#include <liburing.h>
#include <linux/futex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

/// Use nmap to allocate a block of shared memory
WorkerState *worker_create_shared_state() {
    WorkerState *state = calloc(1, sizeof(WorkerState));
    return state;
}

void worker_destroy_shared_state(WorkerState *state) { free(state); }

void *worker_thread(void *arguments) {
    WorkerState *state = arguments;

    printf("Worker thread started\n");
    sleep(5);
    printf("Worker thread finished\n");

    state->answer = 42;

    unsigned int *futex = &state->futex;
    struct io_uring_sqe *sqe;
    struct io_uring_cqe *cqe;
    struct io_uring ring;
    int ret;

    ret = io_uring_queue_init(1, &ring, 0);
    if (ret) {
        fprintf(stderr, "queue init: %d\n", ret);
        return NULL;
    }

    *futex = 1;
    sqe = io_uring_get_sqe(&ring);
    io_uring_prep_futex_wake(sqe, futex, 1, FUTEX_BITSET_MATCH_ANY,
                             FUTEX2_SIZE_U32, 0);
    sqe->user_data = 3;

    io_uring_submit(&ring);

    ret = io_uring_wait_cqe(&ring, &cqe);
    if (ret) {
        fprintf(stderr, "wait: %d\n", ret);
        return NULL;
    }
    io_uring_cqe_seen(&ring, cqe);
    io_uring_queue_exit(&ring);

    return NULL;
}

void spawn_worker_thread(WorkerState *state) {
    pthread_t thread;
    pthread_create(&thread, NULL, worker_thread, state);
}