#pragma once

#include <stdatomic.h>

#include "../bits/freelist.h"
#include "../bits/priority_heap.h"
#include "../protocol.h"
#include "scheduler.h"
#include <stdbool.h>

typedef PriorityHeap(uint32_t) IndexPriorityHeap;

typedef PriorityHeapNode(uint32_t) IndexPriorityHeapNode;

struct PriorityScheduler
{
    struct SchedulerBase base;

    // Jobs storage
    FreeList(struct PrioritySchedulerJob) jobs;

    // List of indexes into jobs ordered by priority
    IndexPriorityHeap* jobs_r;
    IndexPriorityHeap* jobs_w;
    // Prevents jobs ptr from being invalidated by growing.
    struct SRWLock rlock;
    // Prevents jobs ptr from being invalidated by swapping.
    struct SRWLock wlock;
};

struct PrioritySchedulerJob
{
    // Track job progress by atomically incrementing index
    uint64_t block_idx;
    uint64_t block_size;
    uint64_t block_count;

    // Job data
    struct SchedulerJobRecipient* recipient;
    struct ProtocolRequest request;

    SchedulerJobId id; // Job id
    uint32_t rc; // Reference count
};

void scheduler_priority_job_mark_as_done(
    struct PrioritySchedulerJob* job
);

bool scheduler_priority_job_is_done(
    const struct PrioritySchedulerJob* job
);

void scheduler_priority_enter_job(
    struct PriorityScheduler* scheduler,
    uint32_t job_idx
);

void scheduler_priority_leave_job(
    struct PriorityScheduler* scheduler,
    uint32_t job_idx
);

struct PriorityScheduler* scheduler_priority_create(uint32_t default_cap);
void scheduler_priority_destroy(struct PriorityScheduler* scheduler);

SchedulerJobId scheduler_priority_submit(
    struct PriorityScheduler* scheduler,
    const struct ProtocolRequest* request,
    struct SchedulerJobRecipient* recipient
);

void scheduler_priority_cancel(
    const struct PriorityScheduler* scheduler,
    SchedulerJobId job_id
);

bool scheduler_priority_schedule(
    struct PriorityScheduler* scheduler,
    IndexPriorityHeap* local_jobs,
    SchedulerJobId* prev_max_job_id,
    SchedulerJobId* prev_job_id,
    HashDigest target_hash,
    uint32_t* job_idx,
    uint64_t* start,
    uint64_t* end
);

void* scheduler_priority_worker(struct PriorityScheduler* scheduler);
