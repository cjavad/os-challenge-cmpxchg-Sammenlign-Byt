#include "scheduler.h"

#include "../bits/futex.h"
#include "../cache.h"
#include "../protocol.h"

#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

void scheduler_base_init(struct SchedulerBase* scheduler, const uint32_t default_cap) {
    scheduler->waker = 0;
    scheduler->cache = cache_create(default_cap);
    scheduler->job_id = 0;
    scheduler->running = 1;
}

void scheduler_base_close(struct SchedulerBase* scheduler) {
    scheduler->running = 0;
    scheduler_wake_workers(scheduler);
}
void scheduler_base_destroy(const struct SchedulerBase* scheduler) { cache_destroy(scheduler->cache); }

void scheduler_wake_workers(struct SchedulerBase* scheduler) {
    const int64_t s = futex(&scheduler->waker, FUTEX_WAKE, UINT32_MAX, NULL, NULL, 0);

    if (s == -1) {
        __builtin_unreachable();
    }
}
void scheduler_park_worker(struct SchedulerBase* scheduler, const SchedulerJobId job_id) {
    while (scheduler->running) {
        const SchedulerJobId current_job_id = atomic_load(&scheduler->job_id);

        if (current_job_id != job_id) {
            break;
        }

        struct timespec ts = {.tv_sec = 1, .tv_nsec = 0};
        const int64_t s = futex(&scheduler->waker, FUTEX_WAIT, 0, &ts, NULL, 0);

        if (s == -1) {
            if (errno == EAGAIN) {
                break;
            }

            continue;
        }

        break;
    }
}

struct SchedulerJobRecipient* scheduler_create_job_recipient(enum SchedulerJobRecipientType type, const uint32_t data) {
    struct SchedulerJobRecipient* recipient = calloc(1, sizeof(struct SchedulerJobRecipient));
    recipient->type = type;
    recipient->data = data;
    return recipient;
}

void scheduler_destroy_job_recipient(struct SchedulerJobRecipient* recipient) { free(recipient); }

void scheduler_job_notify_recipient(struct SchedulerJobRecipient* recipient, const uint64_t answer) {
    struct ProtocolResponse response = {.answer = answer};
    protocol_response_to_be(&response);

    if (recipient == NULL) {
        return;
    }

    switch (recipient->type) {

    case SCHEDULER_JOB_RECIPIENT_TYPE_FUTEX:
        futex_wake_single(&recipient->futex);
        break;
    case SCHEDULER_JOB_RECIPIENT_TYPE_FD:
        send(recipient->fd, &response, PROTOCOL_RES_SIZE, 0);
        close(recipient->fd);
        scheduler_destroy_job_recipient(recipient);
        break;
    }
}