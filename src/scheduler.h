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

#define SCHEDULER_BLOCK_SIZE (1 << 16)
#define MIN(a, b) ((a) < (b) ? (a) : (b))

struct Job {
    HashDigest hash;
    uint64_t start;
    uint64_t end;
    uint8_t priority;

    volatile uint64_t block_idx;
    uint64_t block_count;

    volatile uint32_t done;

    enum JobType {
        JOB_TYPE_FUTEX,
        JOB_TYPE_FD,
    } type;

    union {
        uint32_t data;
        uint32_t futex;
        int32_t fd;
    } notifier;

    volatile uint64_t answer;

    struct Job* volatile next;
};

typedef struct Job Job;

struct Scheduler {
    bool running;
    Cache* cache;
    Job* volatile next_job;
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
