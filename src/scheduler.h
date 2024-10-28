#pragma once

#include "bits/concat.h"
#include "bits/freelist.h"
#include "bits/futex.h"
#include "bits/priority_heap.h"
#include "bits/prng.h"
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

#define MIN(a, b) ((a) < (b) ? (a) : (b))

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
};

#define scheduler_job_is_done(job) ((job)->block_idx >= (job)->block_count)

typedef PriorityHeap(uint32_t) JobPriorityHeap;

struct Scheduler {
    struct Cache* cache;
    FreeList(struct Job) jobs;
    JobPriorityHeap* sched_jobs;   // Read-only heap
    JobPriorityHeap* sched_jobs_w; // Write-only heap
    uint32_t sched_jobs_version;
    bool running;

    pthread_mutex_t mutex;
    pthread_cond_t waker;
    // These locks need to HEAVILY favor the writer.
    pthread_rwlock_t jobs_swap_lock;
    pthread_rwlock_t jobs_grow_lock;
};

typedef struct Scheduler Scheduler;

#define __PTHREAD_CRITICAL_SECTION(lock_name, lockfn, lock_var)                                                        \
    {                                                                                                                  \
        pthread_##lock_name##_##lockfn(lock_var);

#define __PTHREAD_CRITICAL_SECTION_END(lock_name, lock_var)                                                            \
    pthread_##lock_name##_unlock(lock_var);                                                                            \
    }

#define SCHEDULER_READ_JOBS(scheduler) __PTHREAD_CRITICAL_SECTION(rwlock, rdlock, &(scheduler)->jobs_swap_lock)

#define SCHEDULER_READ_JOBS_END(scheduler) __PTHREAD_CRITICAL_SECTION_END(rwlock, &(scheduler)->jobs_swap_lock)

#define SCHEDULER_READ_PRIO(scheduler) __PTHREAD_CRITICAL_SECTION(rwlock, rdlock, &(scheduler)->jobs_grow_lock)

#define SCHEDULER_READ_PRIO_END(scheduler) __PTHREAD_CRITICAL_SECTION_END(rwlock, &(scheduler)->jobs_grow_lock)

#define SCHEDULER_WRITE_JOBS(scheduler) __PTHREAD_CRITICAL_SECTION(rwlock, wrlock, &(scheduler)->jobs_swap_lock)

#define SCHEDULER_WRITE_JOBS_END(scheduler) __PTHREAD_CRITICAL_SECTION_END(rwlock, &(scheduler)->jobs_swap_lock)

#define SCHEDULER_WRITE_PRIO(scheduler) __PTHREAD_CRITICAL_SECTION(rwlock, wrlock, &(scheduler)->jobs_grow_lock)

#define SCHEDULER_WRITE_PRIO_END(scheduler) __PTHREAD_CRITICAL_SECTION_END(rwlock, &(scheduler)->jobs_grow_lock)

#define SCHEDULER_JOBS_REF_INC(scheduler, job_idx)                                                                     \
    {                                                                                                                  \
        SCHEDULER_READ_JOBS(scheduler)                                                                                 \
        __atomic_add_fetch(&(scheduler)->jobs.data[job_idx].rc, 1, __ATOMIC_RELAXED);                                  \
        SCHEDULER_READ_JOBS_END(scheduler)                                                                             \
    }

// Remove job once it is done and no references are left.
#define __SCHEDULER_JOBS_REF_DEC(scheduler, job_idx, c)                                                                \
    {                                                                                                                  \
        SCHEDULER_READ_JOBS(scheduler)                                                                                 \
        uint32_t __CONCAT(__ref__, c) = __atomic_sub_fetch(&(scheduler)->jobs.data[job_idx].rc, 1, __ATOMIC_RELAXED);  \
        if (scheduler_job_is_done(&(scheduler)->jobs.data[job_idx]) && __CONCAT(__ref__, c) == 0) {                    \
            freelist_remove(&(scheduler)->jobs, job_idx);                                                              \
        }                                                                                                              \
        SCHEDULER_READ_JOBS_END(scheduler)                                                                             \
    }

#define SCHEDULER_JOBS_REF_DEC(scheduler, job_idx) __SCHEDULER_JOBS_REF_DEC(scheduler, job_idx, __COUNTER__)

Scheduler* scheduler_create(uint32_t cap);
void scheduler_destroy(Scheduler* scheduler);

struct JobData* scheduler_create_job_data(enum JobType type, uint32_t data);

/// Submit a new request to the scheduler.
/// JobData has to be accessible from other threads.
void scheduler_submit(Scheduler* scheduler, const struct ProtocolRequest* req, struct JobData* data);

/// Request a new task from the scheduler, returns fall if no task is
/// available.
bool scheduler_schedule(
    Scheduler* scheduler, JobPriorityHeap* local_sched_jobs, uint32_t* local_sched_jobs_version, uint32_t* job_idx,
    uint64_t* start, uint64_t* end
);

/// Notify the scheduler that a job is done.
/// This performs notification of recipients waiting for the job to be done.
void scheduler_job_done(Scheduler* scheduler, uint32_t job_idx, uint64_t answer);
void scheduler_job_notify(struct JobData* data);

void scheduler_close(Scheduler* scheduler);
