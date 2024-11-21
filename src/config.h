#pragma once

#define WORKER_SET_AFFINITY           1
#define WORKER_CONCURRENCY_EXTRA      0
#define WORKER_CONCURRENCY_MULTIPLIER 1

#ifndef SCHEDULER_WAKER_TYPE
#define SCHEDULER_WAKER_TYPE 2
#endif

#define HASH_FUNC      sha256_fullyfused
#define HASH_FUNC_X4   sha256x4_fullyfused_asm
#define HASH_FUNC_X4X2 sha256x4x2_fused

#define REVERSE_FUNC reverse_sha256x4_fullyfused_asm

typedef struct PriorityScheduler SchedulerImpl;
typedef struct EpollServerCtx ServerImplCtx;
