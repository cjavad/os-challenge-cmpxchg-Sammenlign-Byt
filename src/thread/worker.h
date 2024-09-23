#pragma once

#include "../protocol.h"
#include "futex.h"
#include <pthread.h>
#include <stdint.h>

struct WorkerState {
    ProtocolRequest request;
    ProtocolResponse response;

    // Identifier
    union {
	// Can be any uint64_t value.
	uint64_t u64;

	// Most commonly we have a target file descriptor
	// and some buffer id.
	struct {
	    int32_t fd;
	    uint32_t bid;
	};
    } data;

    // Notifier.
    union {
	uint32_t futex;
	int32_t fd;
    } notifier;
};

typedef struct WorkerState WorkerState;

WorkerState* worker_create_shared_state();
void worker_destroy_shared_state(WorkerState* state);

void* worker_thread(void* arguments);
void spawn_worker_thread(WorkerState* state);