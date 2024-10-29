#pragma once

#include "../../experiments/benchmark.h"
#include "sha256x1.h"

#define BENCHMARK_SHA256X1(func)                                                                                       \
    {                                                                                                                  \
        printf("benchmarking %s\n", #func);                                                                            \
        D_BENCHMARK_START(1, SHA256_BENCHMARK_ITERATIONS)                                                              \
        uint8_t hash[SHA256_DIGEST_LENGTH] __attribute__((aligned(16)));                                               \
        uint64_t data[1] __attribute__((aligned(16))) = {0};                                                           \
        D_BENCHMARK_WARMUP(1000000) { (func)(hash, (uint8_t*)data); }                                                  \
        D_BENCHMARK_LOOP_START()                                                                                       \
        (func)(hash, (uint8_t*)data);                                                                                  \
        data[0] += 1;                                                                                                  \
        D_BENCHMARK_LOOP_END()                                                                                         \
        D_BENCHMARK_END()                                                                                              \
    }

#define BENCHMARK_SHA256X1_ALL                                                                                         \
    BENCHMARK_SHA256X1(sha256_custom)                                                                                  \
    BENCHMARK_SHA256X1(sha256_optim)                                                                                   \
    BENCHMARK_SHA256X1(sha256_fused)                                                                                   \
    BENCHMARK_SHA256X1(sha256_fullyfused)
