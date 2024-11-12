#pragma once

#include "../protocol.h"
#include "../bits/atomic.h"
#include "scheduler.h"
#include <stdint.h>

struct LLJob
{
    /* 1st cache line */

    // single writer
    uint32_t done;
    // read only
    uint32_t fumber; // not 0
    // read only
    uint8_t priority;
    // read only
    uint64_t block_count : 56;
    // read only
    uint64_t start;
    // read only
    uint64_t end;
    // read only
    HashDigest hash;

    /* 2nd cache line */

    // single writer
    uint64_t answer;
    // idk lol
    struct SchedulerJobRecipient* recipient;
    // atomic inc
    Atomic(uint64_t) block_idx;

    // TODO :: atomic inc variable where all workers can mark job as seen as
    // done
} __attribute__((aligned(64)));

struct LinkedListScheduler
{
    struct SchedulerBase base;
    struct LLJob* current_job;
    uint32_t block_size;
};

struct LinkedListScheduler* scheduler_linked_list_create(
    const uint32_t default_cap
);

SchedulerJobId scheduler_linked_list_submit(
    struct LinkedListScheduler* scheduler,
    const struct ProtocolRequest* req,
    struct SchedulerJobRecipient* recipient
);

void scheduler_linked_list_cancel(
    struct LinkedListScheduler* scheduler,
    SchedulerJobId job_id
);

void* scheduler_linked_list_worker(struct LinkedListScheduler* scheduler);
