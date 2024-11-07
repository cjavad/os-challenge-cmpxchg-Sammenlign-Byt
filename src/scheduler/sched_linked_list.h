#pragma once

#include "scheduler.h"
#include "../protocol.h"

struct LinkedListScheduler {
    struct SchedulerBase base;
};

void scheduler_linked_list_submit(
    struct LinkedListScheduler* scheduler, const struct ProtocolRequest* req, struct SchedulerJobRecipient* recipient
);

void scheduler_linked_list_cancel(struct LinkedListScheduler* scheduler, SchedulerJobId job_id);

void* scheduler_linked_list_worker(struct LinkedListScheduler* scheduler);