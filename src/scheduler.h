#pragma once

#include "bits/concat.h"
#include "bits/freelist.h"
#include "bits/futex.h"
#include "bits/priority_heap.h"
#include "bits/spin.h"
#include "cache.h"
#include "protocol.h"
#include "sha256/types.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define __minmax_impl(op, a, b, c)                                                                                     \
    ({                                                                                                                 \
        __typeof__(a) __CONCAT(_a, c) = (a);                                                                           \
        __typeof__(b) __CONCAT(_b, c) = (b);                                                                           \
        __CONCAT(_a, c) op __CONCAT(_b, c) ? __CONCAT(_a, c) : __CONCAT(_b, c);                                        \
    })

#define min(a, b) __minmax_impl(<, a, b, __COUNTER__)
#define max(a, b) __minmax_impl(>, a, b, __COUNTER__)

struct JobData {
    enum JobType {
        JOB_TYPE_FUTEX,
        JOB_TYPE_FD,
    } type;

    union {
        uint32_t data;
        uint32_t futex;
        int32_t fd;
    };

    uint64_t answer;
};

struct Job {
    struct ProtocolRequest req;
    struct JobData* data;

    uint64_t block_idx;
    uint64_t block_size;
    uint64_t block_count;

    uint32_t rc;
    uint32_t id;
};

#define scheduler_job_is_done(job) ((job)->block_idx >= (job)->block_count)

typedef PriorityHeap(uint32_t) IndexPriorityHeap;
typedef PriorityHeapNode(uint32_t) IndexPriorityHeapNode;

struct ScheduledJobs {
    IndexPriorityHeap p;
    uint32_t v;
};

struct Scheduler {
    struct Cache* cache;

    FreeList(struct Job) jobs;

    struct ScheduledJobs* sched_jobs_r;
    struct ScheduledJobs* sched_jobs_w;

    struct SRWLock jobs_rwlock; // Prevents jobs ptr from being invalidated by growing.
    struct SRWLock swap_rwlock; // Prevents jobs ptr form being swapped.

    uint32_t futex_waker;
    uint32_t job_id;
    bool running;
};

typedef struct Scheduler Scheduler;

void scheduler_park_thread(Scheduler* scheduler);

void scheduler_wake_all_thread(Scheduler* scheduler);

// Increment job reference count.
void scheduler_job_rc_enter(Scheduler* scheduler, const uint32_t job_idx);

// Decrement job reference count and remove job if no references are left.
void scheduler_job_rc_leave(Scheduler* scheduler, const uint32_t job_idx);

Scheduler* scheduler_create(uint32_t cap);
void scheduler_destroy(Scheduler* scheduler);

struct JobData* scheduler_create_job_data(enum JobType type, uint32_t data);

/// Submit a new request to the scheduler.
/// JobData has to be accessible from other threads.
uint32_t scheduler_submit(Scheduler* scheduler, const struct ProtocolRequest* req, struct JobData* data);
void scheduler_cancel(const Scheduler* scheduler, uint32_t job_id);

/// Request a new task from the scheduler, returns fall if no task is
/// available.
bool scheduler_schedule(
    Scheduler* scheduler, struct ScheduledJobs* local_sched_jobs, uint32_t* last_job_id, HashDigest target,
    uint32_t* job_idx, uint64_t* start, uint64_t* end
);

/// Notify the scheduler that a job is done.
/// This performs notification of recipients waiting for the job to be done.
void scheduler_job_done(Scheduler* scheduler, uint32_t job_idx, uint64_t answer);
void scheduler_job_notify(struct JobData* data);

void scheduler_close(Scheduler* scheduler);
