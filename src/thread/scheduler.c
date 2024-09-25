#include "scheduler.h"

TaskState* scheduler_create_task() {
    TaskState* state = calloc(1, sizeof(TaskState));
    return state;
}

void scheduler_destroy_task(TaskState* state) { free(state); }