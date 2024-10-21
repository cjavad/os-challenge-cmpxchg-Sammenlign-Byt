#include "scheduler.h"


Scheduler* scheduler_create(const uint32_t cap) {
    Scheduler* scheduler = calloc(1, sizeof(Scheduler));

    scheduler->running = true;
    scheduler->cache = cache_create(1024);

    return scheduler;
}

void scheduler_destroy(Scheduler* scheduler) { free(scheduler); }

uint64_t scheduler_submit(Scheduler* scheduler, const struct ProtocolRequest* req, enum JobType type, int32_t data) {
    // Process pending cache entries.
    cache_process_pending(scheduler->cache);

    const uint64_t* cached_answer;

    radix_tree_get(&scheduler->cache->tree, req->hash, SHA256_DIGEST_LENGTH, &cached_answer);

    if (cached_answer != NULL) {
        scheduler_job_notify(*cached_answer, data);
        return 0;
    }

#ifdef DEBUG
    // protocol_debug_print_request(req);
#endif

    Job* job = scheduler->next_job;

    while (job != NULL) {
        if (!job->done) {
            break;
        }

        job = job->next;
    }

    scheduler->next_job = job;

    Job* new_job = calloc(1, sizeof(Job));

    *new_job = (Job){
        .hash = {0},
        .start = req->start,
        .end = req->end,
        .priority = req->priority,
        .block_idx = 0,
        .block_count = ((req->end - req->start) + (SCHEDULER_BLOCK_SIZE - 1)) / SCHEDULER_BLOCK_SIZE,
        .done = 0,
        .type = type,
        .notifier = {.data = data},
        .answer = 0,
        .next = NULL,
    };

    memcpy(&new_job->hash, req->hash, SHA256_DIGEST_LENGTH);

    if (scheduler->next_job == NULL) {
        scheduler->next_job = new_job;
        return 0;
    }

    Job* prev = NULL;

    while (job != NULL) {
        if (job->priority < new_job->priority) {
            break;
        }

        prev = job;
        job = job->next;
    }

    if (prev == NULL) {
        new_job->next = job;
        scheduler->next_job = new_job;
        return 0;
    }

    new_job->next = job;
    prev->next = new_job;

    return 0;
 }

void scheduler_job_done(const Scheduler* scheduler, Job* job) {
    // Notify client of response.
    scheduler_job_notify(job->answer, job->notifier.fd);

    // Store cache entry.
    // TODO: Fix temp workaround where we switch endianness back.
    cache_insert_pending(scheduler->cache, job->hash, job->answer);
}

void scheduler_job_notify(const uint64_t answer, const int32_t fd) {
    struct ProtocolResponse response = {.answer = answer};

    /// We assume this can only be called once.
    protocol_response_to_be(&response);
    send(fd, &response, PROTOCOL_RES_SIZE, 0);
    close(fd);

    // TODO FREE JOB??!?!?
}


void scheduler_close(Scheduler* scheduler) { scheduler->running = false; }
