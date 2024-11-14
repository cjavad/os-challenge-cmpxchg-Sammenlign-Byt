#pragma once

#include "sched_linked_list.h"
#include "sched_priority.h"
#include "sched_rand.h"

#define scheduler_create(scheduler_type, default_cap)                                                                                                                                             \
    _Generic((scheduler_type), struct PriorityScheduler *: scheduler_priority_create, struct RandScheduler *: scheduler_rand_create, struct LinkedListScheduler *: scheduler_linked_list_create)( \
        default_cap                                                                                                                                                                               \
    )

#define scheduler_destroy(scheduler)                                                                                                                                                            \
    _Generic((scheduler), struct PriorityScheduler *: scheduler_priority_destroy, struct RandScheduler *: scheduler_rand_destroy, struct LinkedListScheduler *: scheduler_linked_list_destroy)( \
        scheduler                                                                                                                                                                               \
    )

#define scheduler_submit(scheduler, request, recipient)                                                                                                                                      \
    _Generic((scheduler), struct PriorityScheduler *: scheduler_priority_submit, struct RandScheduler *: scheduler_rand_submit, struct LinkedListScheduler *: scheduler_linked_list_submit)( \
        scheduler,                                                                                                                                                                           \
        request,                                                                                                                                                                             \
        recipient                                                                                                                                                                            \
    )

#define scheduler_cancel(scheduler, job_id)                                                                                                                                                  \
    _Generic((scheduler), struct PriorityScheduler *: scheduler_priority_cancel, struct RandScheduler *: scheduler_rand_cancel, struct LinkedListScheduler *: scheduler_linked_list_cancel)( \
        scheduler,                                                                                                                                                                           \
        job_id                                                                                                                                                                               \
    )

// Returns the worker function for the given scheduler.
#define scheduler_worker_thread(scheduler)                                     \
    ({                                                                         \
        (void* (*)(void*)) _Generic(                                           \
            (scheduler),                                                       \
            struct PriorityScheduler *: scheduler_priority_worker,             \
            struct RandScheduler *: scheduler_rand_worker,                     \
            struct LinkedListScheduler *: scheduler_linked_list_worker         \
        );                                                                     \
    })
