#pragma once

#include "../../experiments/benchmark.h"
#include "sha256x4.h"

#define BENCHMARK_SHA256X4(func)                                               \
    {                                                                          \
        printf("benchmarking %s\n", #func);                                    \
        D_BENCHMARK_START(4, SHA256_BENCHMARK_ITERATIONS)                      \
        uint8_t hash[SHA256_DIGEST_LENGTH * 4] __attribute__((aligned(16)));   \
        uint64_t data[4] __attribute__((aligned(16))) = {0, 1, 2, 3};          \
        D_BENCHMARK_WARMUP(1000000) { (func)(hash, (uint8_t*)data); }          \
        D_BENCHMARK_LOOP_START()                                               \
        (func)(hash, (uint8_t*)data);                                          \
        data[0] += 1;                                                          \
        data[1] += 1;                                                          \
        data[2] += 1;                                                          \
        data[3] += 1;                                                          \
        D_BENCHMARK_LOOP_END()                                                 \
        D_BENCHMARK_END()                                                      \
    }

#define BENCHMARK_SHA256X4_ALL                                                 \
    BENCHMARK_SHA256X4(sha256x4_fullyfused_asm)                                \
    BENCHMARK_SHA256X4(sha256x4_optim)                                         \
    BENCHMARK_SHA256X4(sha256x4_fused)                                         \
    BENCHMARK_SHA256X4(sha256x4_fullyfused)                                    \
    BENCHMARK_SHA256X4(sha256x4_cyclic)
