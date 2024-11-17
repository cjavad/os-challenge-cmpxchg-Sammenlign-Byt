#pragma once

#define WORKER_CONCURRENCY_MULTIPLIER 2

#ifndef SCHEDULER_WAKER_TYPE
#define SCHEDULER_WAKER_TYPE 2
#endif

#define HASH_FUNC      sha256_fullyfused
#define HASH_FUNC_X4   sha256x4_fullyfused
#define HASH_FUNC_X4X2 sha256x4x2_fused

#define REVERSE_FUNC reverse_sha256_x4

typedef struct PriorityScheduler SchedulerImpl;
typedef struct EpollServerCtx ServerImplCtx;
