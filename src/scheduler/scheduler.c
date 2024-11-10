#include "scheduler.h"

#define _BITS_FUTEX_SPIN_LIMIT 50
#define _BITS_FUTEX_TIMEOUT_NS 10000
#include "../bits/futex.h"
#if SCHEDULER_WAKER_TYPE == SCHEDULER_WAKER_TYPE_SPIN
#include <stdatomic.h>
#include <xmmintrin.h>
#elif SCHEDULER_WAKER_TYPE == SCHEDULER_WAKER_TYPE_PTHREAD
#include <pthread.h>
#endif


#include "../cache.h"
#include "../protocol.h"

#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>


void scheduler_base_init(
    struct SchedulerBase* scheduler,
    const uint32_t default_cap
) {
#if SCHEDULER_WAKER_TYPE == SCHEDULER_WAKER_TYPE_SPIN || SCHEDULER_WAKER_TYPE == SCHEDULER_WAKER_TYPE_FUTEX
    scheduler->waker = 0;
#elif SCHEDULER_WAKER_TYPE == SCHEDULER_WAKER_TYPE_PTHREAD
    scheduler->waker_lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    scheduler->waker_cond = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
#endif
    scheduler->cache = cache_create(default_cap);
    scheduler->job_id = 0;
    scheduler->running = 1;
}

void scheduler_base_close(
    struct SchedulerBase* scheduler
) {
    scheduler->running = 0;
    scheduler_wake_workers(scheduler);
}

void scheduler_base_destroy(
    struct SchedulerBase* scheduler
) {
#if SCHEDULER_WAKER_TYPE == SCHEDULER_WAKER_TYPE_PTHREAD
    pthread_mutex_destroy(&scheduler->waker_lock);
    pthread_cond_destroy(&scheduler->waker_cond);
#endif
    cache_destroy(scheduler->cache);
}

inline void scheduler_wake_workers(
    struct SchedulerBase* scheduler
) {
#if SCHEDULER_WAKER_TYPE == SCHEDULER_WAKER_TYPE_SPIN
    atomic_store(&scheduler->waker, 0);
#elif SCHEDULER_WAKER_TYPE == SCHEDULER_WAKER_TYPE_FUTEX
    futex_wake(&scheduler->waker);
#elif SCHEDULER_WAKER_TYPE == SCHEDULER_WAKER_TYPE_PTHREAD
    pthread_cond_broadcast(&scheduler->waker_cond);
#endif
}

inline void scheduler_yield_worker(struct SchedulerBase* scheduler) {
#if SCHEDULER_WAKER_TYPE == SCHEDULER_WAKER_TYPE_SPIN
    const uint32_t position = atomic_fetch_add(&scheduler->waker, 1);
    while (atomic_load(&scheduler->waker) > position) {
        _mm_pause();
    }
#elif SCHEDULER_WAKER_TYPE == SCHEDULER_WAKER_TYPE_FUTEX
    futex_wait(&scheduler->waker);
#elif SCHEDULER_WAKER_TYPE == SCHEDULER_WAKER_TYPE_PTHREAD
    pthread_mutex_lock(&scheduler->waker_lock);
    pthread_cond_wait(&scheduler->waker_cond, &scheduler->waker_lock);
    pthread_mutex_unlock(&scheduler->waker_lock);
#endif
}

struct SchedulerJobRecipient* scheduler_create_job_recipient(
    const enum SchedulerJobRecipientType type,
    const uint32_t data
) {
    struct SchedulerJobRecipient* recipient =
        calloc(1, sizeof(struct SchedulerJobRecipient));
    recipient->type = type;
    recipient->data = data;
    return recipient;
}

void scheduler_destroy_job_recipient(
    struct SchedulerJobRecipient* recipient
) {
    free(recipient);
}

void scheduler_job_notify_recipient(
    struct SchedulerJobRecipient* recipient,
    const uint64_t answer
) {
    struct ProtocolResponse response = {.answer = answer};
    protocol_response_to_be(&response);

    if (recipient == NULL) {
        return;
    }

    switch (recipient->type) {

    case SCHEDULER_JOB_RECIPIENT_TYPE_FUTEX:
        futex_wake(&recipient->futex);
        break;
    case SCHEDULER_JOB_RECIPIENT_TYPE_FD:
        send(recipient->fd, &response, PROTOCOL_RES_SIZE, 0);
        close(recipient->fd);
        scheduler_destroy_job_recipient(recipient);
        break;
    }
}