#pragma once

#include "../protocol.h"

struct TaskState {
    ProtocolRequest request;
    ProtocolResponse response;

    union {
	uint32_t futex;
	int32_t fd;
    } data;
};

typedef struct TaskState TaskState;

TaskState* scheduler_create_task();
void scheduler_destroy_task(TaskState* state);