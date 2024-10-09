#include "scheduler.h"

#include "futex.h"

#define SCHEDULER_BLOCK_SIZE (1000000)

Scheduler* scheduler_create(const uint32_t cap) {
    Scheduler* scheduler = calloc(1, sizeof(Scheduler));

    scheduler->block_size = SCHEDULER_BLOCK_SIZE;

    pthread_mutex_init(&scheduler->mutex, NULL);

    scheduler->jobs = calloc(cap, sizeof(Job));
    scheduler->job_cap = cap;
    scheduler->job_len = 0;

    scheduler->priority_sum = 0;

    scheduler->task_idx = 0;
    scheduler->priority_idx = 0;

    scheduler->prng_state = 0xbeefbaad;

    return scheduler;
}

void scheduler_destroy(Scheduler* scheduler) {
    pthread_mutex_destroy(&scheduler->mutex);
    free(scheduler->jobs);
    free(scheduler);
}

JobData* scheduler_create_job_data(JobType type, const uint32_t data) {
    JobData* job_data = calloc(1, sizeof(JobData));
    job_data->type = type;
    job_data->data = data;
    return job_data;
}

static void scheduler_grow_jobs(Scheduler* scheduler) {
    const uint32_t new_cap = scheduler->job_cap * 2;
    Job* new_jobs = reallocarray(scheduler->jobs, new_cap, sizeof(Job));

    if (new_jobs == NULL) {
        fprintf(stderr, "Failed to grow jobs array - segfault imminent\n");
        return;
    }

    scheduler->jobs = new_jobs;
    scheduler->job_cap = new_cap;
}

static void scheduler_push_job(Scheduler* scheduler, const ProtocolRequest* req, uint64_t id, JobData* data) {
    if (scheduler->job_len >= scheduler->job_cap) {
        scheduler_grow_jobs(scheduler);
    }

    Job* job = &scheduler->jobs[scheduler->job_len];
    memcpy(&job->hash, req->hash, sizeof(HashDigest));
    job->start = req->start;
    job->end = req->end;
    job->priority = req->priority;
    job->id = id;
    job->data = data;

    scheduler->job_len += 1;
}

static void scheduler_remove_swap_job(Scheduler* scheduler, uint32_t idx) {
    Job* task = &scheduler->jobs[idx];
    Job* last = &scheduler->jobs[scheduler->job_len - 1];

    memcpy(task, last, sizeof(Job));
    scheduler->job_len -= 1;
}

uint64_t scheduler_submit(Scheduler* scheduler, ProtocolRequest* req, JobData* data) {
    pthread_mutex_lock(&scheduler->mutex);

    // If the priority is 0, set it to 1
    if (req->priority == 0) {
        req->priority = 1;
    }

    const uint64_t job_id = scheduler->next_job_id++;

    scheduler_push_job(scheduler, req, job_id, data);
    scheduler->priority_sum += req->priority;

    pthread_mutex_unlock(&scheduler->mutex);

    return job_id;
}

bool scheduler_terminate(Scheduler* scheduler, const uint64_t job_id) {
    bool found = false;

    pthread_mutex_lock(&scheduler->mutex);

    for (uint32_t i = 0; i < scheduler->job_len; i++) {
        const Job* job = &scheduler->jobs[i];

        if (job->id == job_id) {
            scheduler_remove_swap_job(scheduler, i);
            scheduler->priority_sum -= job->priority;

            scheduler->task_idx = 0;
            scheduler->priority_idx = 0;
            found = true;
            break;
        }
    }

    pthread_mutex_unlock(&scheduler->mutex);

    return found;
}

void scheduler_job_done(Scheduler* scheduler, const Task* task, ProtocolResponse* response) {
    /// We assume this can only be called once.
    task->data->response = *response;

    switch (task->data->type) {
    case JOB_TYPE_FD:
        protocol_response_to_be(response);
        send(task->data->fd, response, PROTOCOL_RES_SIZE, 0);
        close(task->data->fd);
        break;

    case JOB_TYPE_FUTEX:
        futex_post(&task->data->futex);
        break;
    }
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))

bool scheduler_schedule(Scheduler* scheduler, Task* task) {
    pthread_mutex_lock(&scheduler->mutex);

    if (scheduler->job_len == 0) {
        pthread_mutex_unlock(&scheduler->mutex);
        return false;
    }

    if (scheduler->priority_sum == 0) {
        printf("scheduler->priority_sum == 0!!! But job_len == %d\n", scheduler->job_len);
        pthread_mutex_unlock(&scheduler->mutex);
        return false;
    }

    uint32_t offset = xorshift32_prng_next(&scheduler->prng_state) % scheduler->priority_sum;

    for (;;) {
        Job* current = &scheduler->jobs[scheduler->task_idx];
        uint32_t remaining = current->priority - scheduler->priority_idx;

        if (remaining < offset) {
            scheduler->task_idx = (scheduler->task_idx + 1) % scheduler->job_len;
            scheduler->priority_idx = 0;

            offset -= remaining;
            continue;
        }

        scheduler->priority_idx += offset;
        break;
    }

    Job* current = &scheduler->jobs[scheduler->task_idx];

    task->job_id = current->id;
    task->start = current->start;
    task->end = current->start + scheduler->block_size;
    task->end = MIN(current->end, task->end);

    memcpy(task->hash, current->hash, sizeof(HashDigest));
    task->data = current->data;

    current->start = task->end;

    if (current->start == current->end) {
        scheduler_remove_swap_job(scheduler, scheduler->task_idx);
        scheduler->priority_sum -= current->priority;

        // reset the scheduler state, or segfaults be upon us
        scheduler->task_idx = 0;
        scheduler->priority_idx = 0;
    }

    pthread_mutex_unlock(&scheduler->mutex);

    return true;
}

void scheduler_empty(Scheduler* scheduler) {
    pthread_mutex_lock(&scheduler->mutex);

    scheduler->job_len = 0;
    scheduler->priority_sum = 0;

    pthread_mutex_unlock(&scheduler->mutex);
}
