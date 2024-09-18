#pragma once

#include "../protocol.h"
#include "futex.h"
#include <pthread.h>
#include <stdint.h>

typedef struct WorkerState {
    ProtocolRequest request;
    ProtocolResponse response;

    // Notifier.
    union {
        uint32_t futex;
        int32_t fd;
    };
} WorkerState;

WorkerState *worker_create_shared_state();
void worker_destroy_shared_state(WorkerState *state);

void *worker_thread(void *arguments);
void spawn_worker_thread(WorkerState *state);