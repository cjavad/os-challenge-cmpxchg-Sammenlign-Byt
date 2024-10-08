#pragma once

#include "../protocol.h"

#include <pthread.h>
#include <stdbool.h>

union JobData {
    uint32_t futex;
    int32_t fd;
};

typedef union JobData JobData;

struct Task {
    HashDigest hash;

    uint64_t start;
    uint64_t end;

    /* unique identifier for the job */
    uint64_t job_id;

    JobData data;
};

typedef struct Task Task;

struct Job {
    HashDigest hash;

    uint64_t start;
    uint64_t end;

    /* unique identifier for the job */
    uint64_t id;

    uint8_t priority;

    JobData data;
};

typedef struct Job Job;

struct Scheduler {
    uint64_t block_size;

    pthread_mutex_t mutex;

    /* task list */
    Job* jobs;
    uint32_t job_len;
    uint32_t job_cap;

    uint64_t next_job_id;

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
uint64_t scheduler_submit(Scheduler* scheduler, const ProtocolRequest* req, JobData data);

/// Terminate a job by its unique identifier.
void scheduler_terminate(Scheduler* scheduler, uint64_t job_id);

/// Request a new task from the scheduler, returns fall if no task is
/// available.
bool scheduler_schedule(Scheduler* scheduler, Task* task);

void scheduler_empty(Scheduler* scheduler);
