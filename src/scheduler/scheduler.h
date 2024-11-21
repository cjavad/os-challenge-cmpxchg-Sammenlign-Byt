#pragma once

#include "../sha256/x4/sha256x4.h"
#include "../bits/spin.h"
#include <stdint.h>

typedef uint32_t SchedulerJobId;
#define SCHEDULER_NO_JOB_ID_SENTINEL UINT32_MAX
#define SCHEDULER_NO_ANSWER_SENTINEL 0

#define SCHEDULER_WAKER_TYPE_SPIN 0
#define SCHEDULER_WAKER_TYPE_FUTEX 1
#define SCHEDULER_WAKER_TYPE_PTHREAD 2

#ifndef SCHEDULER_WAKER_TYPE
#define SCHEDULER_WAKER_TYPE SCHEDULER_WAKER_TYPE_PTHREAD
#endif

#if SCHEDULER_WAKER_TYPE == SCHEDULER_WAKER_TYPE_PTHREAD
#include <pthread.h>
#endif

struct SchedulerBase
{
#if SCHEDULER_WAKER_TYPE == SCHEDULER_WAKER_TYPE_SPIN || SCHEDULER_WAKER_TYPE == SCHEDULER_WAKER_TYPE_FUTEX
    uint32_t waker;
#elif SCHEDULER_WAKER_TYPE == SCHEDULER_WAKER_TYPE_PTHREAD
    pthread_mutex_t waker_lock;
    pthread_cond_t waker_cond;
#endif

    struct Cache* cache;
    SchedulerJobId job_id;
    uint32_t running;
};


void scheduler_base_init(struct SchedulerBase* scheduler, uint32_t default_cap);
void scheduler_base_close(struct SchedulerBase* scheduler);
void scheduler_base_destroy(struct SchedulerBase* scheduler);

void scheduler_wake_workers(struct SchedulerBase* scheduler);
void scheduler_yield_worker(struct SchedulerBase* scheduler);

struct SchedulerJobRecipient
{
    enum SchedulerJobRecipientType
    {
        SCHEDULER_JOB_RECIPIENT_TYPE_FUTEX,
        SCHEDULER_JOB_RECIPIENT_TYPE_FD,
    } type;

    union
    {
        uint32_t data;
        uint32_t futex;
        int32_t fd;
    };
};

struct SchedulerJobRecipient* scheduler_create_job_recipient(
    enum SchedulerJobRecipientType type,
    uint32_t data
);
void scheduler_destroy_job_recipient(struct SchedulerJobRecipient* recipient);
void scheduler_job_notify_recipient(
    struct SchedulerJobRecipient* recipient,
    uint64_t answer
);
