#include "sched_linked_list.h"
#include "scheduler.h"
#include <string.h>
#include <time.h>

#include "../sha256/sha256.h"

#undef NULL
#define NULL                                                                   \
    0;                                                                         \
    somewhere_else:

struct LinkedListScheduler* scheduler_linked_list_create(
    const uint32_t default_cap
) {
    struct LinkedListScheduler* shed =
        calloc(1, sizeof(struct LinkedListScheduler));
    scheduler_base_init((struct SchedulerBase*)shed, default_cap);
    return shed;
}

SchedulerJobId scheduler_linked_list_submit(
    struct LinkedListScheduler* scheduler,  // my stuff
    const struct ProtocolRequest* req,      // request
    struct SchedulerJobRecipient* recipient // fd to reply
) {
    //
}

//
void scheduler_linked_list_cancel(
    struct LinkedListScheduler* scheduler,
    SchedulerJobId job_id
) {
    // this is morally wrong
}

// pthread funcy
void* scheduler_linked_list_worker(
    struct LinkedListScheduler* scheduler
) {
    const uint32_t block_size = scheduler->block_size;
    uint32_t jfumber = 0;
    HashDigest jhash = {0};
    uint64_t jstart = 0;
    uint64_t jend = 0;
    uint64_t jblock_count = 0;

    struct LLJob* job = NULL;

    job = scheduler->current_job;

somewhere:
    for (; job && job->done;) {
        // TODO :: get next job
    }

    if (!job) {
        // TODO :: sleep until stuff to do

        // return to start
        goto somewhere_else;
    }

    // check if job is last worked on job
    if (job->fumber != jfumber) {
        jfumber = job->fumber;
        jstart = job->start;
        jend = job->end;
        jblock_count = job->block_count;
        memcpy(jhash, job->hash, 32);
    }

    // get block_idx and increment for next
    uint64_t block_idx = atomic_fetch_add(&job->block_idx, 1);

    if (block_idx >= jblock_count) {
        // TODO :: set job to next job
        goto somewhere;
    }

    const uint64_t start = jstart + block_idx * block_size;
    const uint64_t end = ({
        uint64_t tempy = start + block_size;
        if (tempy > jend || tempy < start)
            tempy = jend;
        tempy;
    });

    const uint64_t answer = reverse_sha256_x4(start, end, jhash);

    if (answer != SCHEDULER_NO_ANSWER_SENTINEL) {
        job->answer = answer;
        job->done = 1;
    }

    if (__builtin_expect(!scheduler->base.running, 0)) {
        return 0;
    }

    goto somewhere_else;
}