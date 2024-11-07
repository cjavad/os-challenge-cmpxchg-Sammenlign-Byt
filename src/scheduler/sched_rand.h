#pragma once

#include "../protocol.h"
#include "scheduler.h"

struct RandScheduler {
    struct SchedulerBase base;
};

void scheduler_rand_submit(
    struct RandScheduler* scheduler,
    const struct ProtocolRequest* req,
    struct SchedulerJobRecipient* recipient
);

void scheduler_rand_cancel(
    struct RandScheduler* scheduler,
    SchedulerJobId job_id
);

void* scheduler_rand_worker(struct RandScheduler* scheduler);