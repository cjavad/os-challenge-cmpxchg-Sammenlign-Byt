#pragma once

#include "../bits/atomic.h"
#include "../protocol.h"
#include "scheduler.h"
#include <stdint.h>

#define SCHEDULER_LL_BLOCK_SIZE (1 << 16)

struct LLJob {
    /* 1st cache line */

    // single writer
    uint32_t done;
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
    struct LLJob* next;

    // single writer
    uint64_t answer;
    // idk lol
    struct SchedulerJobRecipient* recipient;
    // atomic inc
    Atomic(uint64_t) block_idx;

    // TODO :: atomic inc variable where all workers can mark job as seen as
    // done
} __attribute__((aligned(64)));

struct LinkedListScheduler {
    struct SchedulerBase base;
    struct LLJob* current_job;
};

struct LinkedListScheduler* scheduler_linked_list_create(uint32_t default_cap);

void scheduler_linked_list_destroy(struct LinkedListScheduler* scheduler);

SchedulerJobId scheduler_linked_list_submit(
    struct LinkedListScheduler* scheduler,
    const struct ProtocolRequest* request,
    struct SchedulerJobRecipient* recipient
);

void scheduler_linked_list_cancel(
    struct LinkedListScheduler* scheduler,
    SchedulerJobId job_id
);

void* scheduler_linked_list_worker(struct LinkedListScheduler* scheduler);
