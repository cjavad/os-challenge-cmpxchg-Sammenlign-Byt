#pragma once

#include "bits/futex.h"
#include "bits/prng.h"
#include "bits/ringbuffer.h"
#include "bits/vec.h"
#include "cache.h"
#include "protocol.h"
#include "sha256/types.h"
#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define SCHEDULER_BLOCK_SIZE (1024)
#define MIN(a, b)            ((a) < (b) ? (a) : (b))

struct Job {
    uint64_t block_idx;
    uint32_t done;

    union {
        uint32_t data;
        uint32_t futex;
        int32_t fd;
    } notifier;

    HashDigest hash;
    uint64_t start;
    uint64_t end;
    uint8_t priority;

    enum JobType {
        JOB_TYPE_FUTEX,
        JOB_TYPE_FD,
    } type;

    uint64_t block_count;

    uint64_t answer;

    struct Job* next;
};

typedef struct Job Job;

struct Scheduler {
    bool running;
    Cache* cache;
    Job* next_job;

    pthread_mutex_t mutex;
    pthread_cond_t waker;
};

typedef struct Scheduler Scheduler;

Scheduler* scheduler_create(uint32_t cap);
void scheduler_destroy(Scheduler* scheduler);

/// Submit a new request to the scheduler.
/// JobData has to be accessible from other threads.
uint64_t scheduler_submit(Scheduler* scheduler, const struct ProtocolRequest* req, enum JobType type, int32_t data);

/// Notify the scheduler that a job is done.
/// This performs notification of recipients waiting for the job to be done.
void scheduler_job_done(const Scheduler* scheduler, Job* job);
void scheduler_job_notify(uint64_t answer, int32_t fd);

/// Request a new task from the scheduler, returns fall if no task is
/// available.
bool scheduler_schedule(Scheduler* scheduler, Job** job);

void scheduler_close(Scheduler* scheduler);
