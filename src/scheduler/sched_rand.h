#pragma once

#include "../bits/ringbuffer.h"
#include "../bits/vec.h"
#include "../protocol.h"
#include "scheduler.h"

struct RandSchedulerJob {
    HashDigest hash;
    uint64_t start;
    uint64_t end;
    uint8_t priority;
    SchedulerJobId id;
    struct SchedulerJobRecipient* recipient;
};

struct RandSchedulerTask {
    HashDigest hash;
    uint64_t start;
    uint64_t end;
    SchedulerJobId id;
    struct SchedulerJobRecipient* recipient;
};

struct RandScheduler {
    struct SchedulerBase base;
    uint64_t block_size;

    pthread_mutex_t lock;
    RingBuffer(struct RandSchedulerJob) pending_jobs;
    Vec(struct RandSchedulerJob) jobs;

    SchedulerJobId next_job_id;
    uint32_t priority_sum;
    uint32_t task_idx;
    uint32_t priority_idx;
    uint32_t prng_state;
};

struct RandScheduler* scheduler_rand_create(uint32_t default_cap);
void scheduler_rand_destroy(struct RandScheduler* scheduler);

SchedulerJobId scheduler_rand_submit(
    struct RandScheduler* scheduler,
    const struct ProtocolRequest* request,
    struct SchedulerJobRecipient* recipient
);

void scheduler_rand_cancel(
    struct RandScheduler* scheduler,
    SchedulerJobId job_id
);

void* scheduler_rand_worker(struct RandScheduler* scheduler);