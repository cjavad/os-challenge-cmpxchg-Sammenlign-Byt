#pragma once

#include "../cache.h"
#include "../prng.h"
#include "../protocol.h"
#include "../sha256/sha256.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

enum JobType {
    JOB_TYPE_FUTEX,
    JOB_TYPE_FD,
};

typedef enum JobType JobType;

struct JobData {
    JobType type;

    union {
        uint32_t data;
        uint32_t futex;
        int32_t fd;
    };

    ProtocolResponse response;
};

typedef struct JobData JobData;

struct Task {
    HashDigest hash;

    uint64_t start;
    uint64_t end;

    /* unique identifier for the job */
    uint64_t job_id;

    JobData* data;
};

typedef struct Task Task;

struct Job {
    HashDigest hash;

    uint64_t start;
    uint64_t end;

    /* unique identifier for the job */
    uint64_t id;

    uint8_t priority;

    JobData* data;
};

typedef struct Job Job;

struct Scheduler {
    uint64_t block_size;

    bool running;
    pthread_cond_t waker;
    pthread_mutex_t r_mutex;
    pthread_mutex_t w_mutex;

    /* cache */
    Cache* cache;

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

JobData* scheduler_create_job_data(JobType type, uint32_t data);

/// Submit a new request to the scheduler.
/// JobData has to be accessible from other threads.
uint64_t scheduler_submit(Scheduler* scheduler, ProtocolRequest* req, JobData* data);

/// Notify the scheduler that a job is done.
/// This performs notification of recipients waiting for the job to be done.
void scheduler_job_done(const Scheduler* scheduler, const Task* task, ProtocolResponse* response);

/// Terminate a job by its unique identifier.
bool scheduler_terminate(Scheduler* scheduler, uint64_t job_id);

/// Request a new task from the scheduler, returns fall if no task is
/// available.
bool scheduler_schedule(Scheduler* scheduler, Task* task);

void scheduler_close(Scheduler* scheduler);

void scheduler_debug_print(const Scheduler* scheduler);
void scheduler_debug_print_job(const Job* job);