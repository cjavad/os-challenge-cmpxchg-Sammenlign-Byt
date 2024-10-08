#pragma once

#include "../protocol.h"

#include <pthread.h>
#include <stdbool.h>

union TaskData {
    uint32_t futex;
    int32_t fd;
};

typedef union TaskData TaskData;

struct Task {
    HashDigest hash;

    uint64_t start;
    uint64_t end;

    TaskData data;
};

typedef struct Task Task;

struct TaskState {
    HashDigest hash;

    uint64_t start;
    uint64_t end;

    uint8_t priority;

    TaskData data;
};

typedef struct TaskState TaskState;

struct Scheduler {
    uint64_t block_size;

    pthread_mutex_t mutex;

    /* task list */
    TaskState* tasks;
    uint32_t task_len;
    uint32_t task_cap;

    /* priority sum */
    uint32_t priority_sum;

    /* task index */
    uint32_t task_idx;
    uint32_t priority_idx;

    /* prng state */
    uint32_t prng_state;
};

typedef struct Scheduler Scheduler;

Scheduler* scheduler_create(uint32_t cap);
void scheduler_destroy(Scheduler* scheduler);

/// Submit a new request to the scheduler.
void scheduler_submit(Scheduler* scheduler, const ProtocolRequest* req, TaskData data);

/// Request a new task from the scheduler, returns fall if no task is
/// available.
bool scheduler_schedule(Scheduler* scheduler, Task* task);

void scheduler_empty(Scheduler* scheduler);
