#pragma once
#include "../../experiments/benchmark.h"
#include "sha256x4x2.h"

#define BENCHMARK_SHA256X4X2(func)                                             \
    {                                                                          \
        printf("benchmarking %s\n", #func);                                    \
        D_BENCHMARK_START(8, SHA256_BENCHMARK_ITERATIONS)                      \
        uint8_t hash[SHA256_DIGEST_LENGTH * 8] __attribute__((aligned(16)));   \
        uint64_t data[8]                                                       \
            __attribute__((aligned(16))) = {0, 1, 2, 3, 4, 5, 6, 7};           \
        D_BENCHMARK_WARMUP(1000000) { (func)(hash, (uint8_t*)data); }          \
        D_BENCHMARK_LOOP_START()                                               \
        (func)(hash, (uint8_t*)data);                                          \
        data[0] += 1;                                                          \
        data[1] += 1;                                                          \
        data[2] += 1;                                                          \
        data[3] += 1;                                                          \
        data[4] += 1;                                                          \
        data[5] += 1;                                                          \
        data[6] += 1;                                                          \
        data[7] += 1;                                                          \
        D_BENCHMARK_LOOP_END()                                                 \
        D_BENCHMARK_END()                                                      \
    }

#define BENCHMARK_SHA256X4X2_ALL                                               \
    BENCHMARK_SHA256X4X2(sha256x4x2_optim)                                     \
    BENCHMARK_SHA256X4X2(sha256x4x2_fused)
