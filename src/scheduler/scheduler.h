#pragma once

#include "../bits/spin.h"

#include <stdint.h>

typedef uint32_t SchedulerJobId;
#define SCHEDULER_NO_JOB_ID_SENTINEL UINT32_MAX
#define SCHEDULER_NO_ANSWER_SENTINEL 0

struct SchedulerBase {
    uint32_t waker;
    struct Cache* cache;
    SchedulerJobId job_id;
    uint32_t running;
};

struct SchedulerJobRecipient {
    enum SchedulerJobRecipientType {
        SCHEDULER_JOB_RECIPIENT_TYPE_FUTEX,
        SCHEDULER_JOB_RECIPIENT_TYPE_FD,
    } type;

    union {
        uint32_t data;
        uint32_t futex;
        int32_t fd;
    };
};

void scheduler_base_init(struct SchedulerBase* scheduler, uint32_t default_cap);
void scheduler_base_close(struct SchedulerBase* scheduler);
void scheduler_base_destroy(const struct SchedulerBase* scheduler);

void scheduler_wake_workers(struct SchedulerBase* scheduler);
void scheduler_park_worker(struct SchedulerBase* scheduler, SchedulerJobId job_id);

struct SchedulerJobRecipient* scheduler_create_job_recipient(enum SchedulerJobRecipientType type, uint32_t data);
void scheduler_destroy_job_recipient(struct SchedulerJobRecipient* recipient);
void scheduler_job_notify_recipient(struct SchedulerJobRecipient* recipient, uint64_t answer);