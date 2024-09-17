#include "worker.h"

#include <liburing.h>
#include <linux/futex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

/// Use nmap to allocate a block of shared memory
WorkerState* worker_create_shared_state() {
    WorkerState* state = calloc(1, sizeof(WorkerState));
    return state;
}

void worker_destroy_shared_state(WorkerState* state) { free(state); }

void* worker_thread(void* arguments) {
    WorkerState* state = arguments;

    state->response.answer = reverse_hash(
        state->request.start, state->request.end, state->request.hash
    );

#if USE_IO_URING

    unsigned int* futex = &state->futex;
    futex_post(futex);

#else

#include <sys/eventfd.h>

    eventfd_write(state->fd, 1);
#endif

    return NULL;
}

void spawn_worker_thread(WorkerState* state) {
    pthread_t thread;
    pthread_create(&thread, NULL, worker_thread, state);
}