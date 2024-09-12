#pragma once

#include "futex.h"
#include <pthread.h>
#include <stdint.h>

typedef struct WorkerState {
    uint64_t answer;
    uint32_t futex;
} WorkerState;

WorkerState *worker_create_shared_state();
void worker_destroy_shared_state(WorkerState *state);

void *worker_thread(void *arguments);
void spawn_worker_thread(WorkerState *state);