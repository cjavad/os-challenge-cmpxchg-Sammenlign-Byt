#include "sched_linked_list.h"
#include "../cache.h"
#include "../config.h"
#include "scheduler.h"

#include "../sha256/sha256.h"
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

struct LinkedListScheduler* scheduler_linked_list_create(
    const uint32_t default_cap
) {
    struct LinkedListScheduler* scheduler =
        aligned_alloc(64, sizeof(struct LinkedListScheduler));

    memset(scheduler, 0, sizeof(struct LinkedListScheduler));
    scheduler_base_init((struct SchedulerBase*)scheduler, default_cap);
    return scheduler;
}

void scheduler_linked_list_destroy(
    struct LinkedListScheduler* scheduler
) {
    scheduler_base_destroy(&scheduler->base);
    free(scheduler);
}

SchedulerJobId scheduler_linked_list_submit(
    struct LinkedListScheduler* scheduler,
    // my stuff
    const struct ProtocolRequest* request,
    // request
    struct SchedulerJobRecipient* recipient // fd to reply
) {
    cache_process_pending(scheduler->base.cache);

    uint64_t* cached_answer;

    radix_tree_get(
        &scheduler->base.cache->tree,
        request->hash,
        SHA256_DIGEST_LENGTH,
        &cached_answer
    );

    if (cached_answer != NULL) {
        scheduler_job_notify_recipient(recipient, *cached_answer);
        return SCHEDULER_NO_JOB_ID_SENTINEL;
    }

    struct LLJob* job = scheduler->current_job;

    while (job != NULL && !job->done) {
        job = job->next;
    }

    scheduler->current_job = job;

    struct LLJob* new_job = aligned_alloc(64, sizeof(struct LLJob));

    *new_job = (struct LLJob){
        .start = request->start,
        .end = request->end,
        .priority = request->priority,
        .block_idx = 0,
        .block_count =
            (request->end - request->start) / SCHEDULER_LL_BLOCK_SIZE,
        .done = 0,
        .recipient = recipient,
        .answer = 0,
        .next = NULL,
    };

    memcpy(new_job->hash, request->hash, sizeof(HashDigest));

    if (job == NULL) {
        scheduler->current_job = new_job;
        goto wake;
    }

    struct LLJob* prev_job = NULL;

    while (job != NULL && job->priority <= new_job->priority) {
        prev_job = job;
        job = job->next;
    }

    if (prev_job == NULL) {
        new_job->next = job;
        scheduler->current_job = new_job;
        goto wake;
    }

    new_job->next = job;
    prev_job->next = new_job;

wake:
    scheduler_wake_workers(&scheduler->base);

    return 0;
}

void scheduler_linked_list_cancel(
    struct LinkedListScheduler* scheduler,
    const SchedulerJobId job_id
) {
    // this is morally wrong
    (void)scheduler;
    (void)job_id;
}

// pthread funcy
void* scheduler_linked_list_worker(
    struct LinkedListScheduler* scheduler
) {
    HashDigest jhash = {0};
    uint64_t jstart = 0;
    uint64_t jend = 0;
    uint64_t jblock_count = 0;

    struct LLJob* job = NULL;
    struct LLJob* prev_job = NULL;

start:
    if (!scheduler->base.running) {
        return NULL;
    }

    job = scheduler->current_job;

next_job:
    while (job != NULL && job->done) {
        job = job->next;
    }

    if (job == NULL) {
        scheduler_yield_worker(&scheduler->base);
        goto start;
    }

    // check if job is last worked on job
    // TODO :: replace fumber with ptr cmp
    if (job != prev_job) {
        jstart = job->start;
        jend = job->end;
        jblock_count = job->block_count;
        memcpy(jhash, job->hash, sizeof(HashDigest));
        prev_job = job;
    }

    // get block_idx and increment for next
    const uint64_t block_idx = atomic_fetch_add(&job->block_idx, 1);

    if (block_idx >= jblock_count) {
        goto next_job;
    }

    const uint64_t start = jstart + block_idx * SCHEDULER_LL_BLOCK_SIZE;
    const uint64_t end = ({
        uint64_t tempy = start + SCHEDULER_LL_BLOCK_SIZE;
        if (tempy > jend || tempy < start)
            tempy = jend;
        tempy;
    });

    const uint64_t answer = REVERSE_FUNC(start, end, jhash);

    if (answer != SCHEDULER_NO_ANSWER_SENTINEL) {
        job->answer = answer;
        job->done = 1;
        scheduler_job_notify_recipient(job->recipient, answer);
        cache_insert_pending(scheduler->base.cache, jhash, answer);
    }

    if (__builtin_expect(!scheduler->base.running, 0)) {
        return NULL;
    }

    goto start;
}