#pragma once

#include "../../experiments/benchmark.h"
#include "sha256x8.h"

#define BENCHMARK_SHA256X8(func)                                                                                       \
    {                                                                                                                  \
        uint8_t hash[SHA256_DIGEST_LENGTH * 8] __attribute__((aligned(16)));                                           \
        BENCHMARK_START                                                                                                \
        for (uint64_t i = 0; i < SHA256_BENCHMARK_ITERATIONS; i += 8) {                                                \
            uint64_t data[8] __attribute__((aligned(16))) = {i, i + 1, i + 2, i + 3, i + 4, i + 5, i + 6, i + 7};      \
            (func)(hash, (uint8_t*)data);                                                                              \
        }                                                                                                              \
        BENCHMARK_END(func, SHA256_BENCHMARK_ITERATIONS)                                                               \
        printf("Hash: ");                                                                                              \
        print_hash(&hash[SHA256_DIGEST_LENGTH * 7]);                                                                   \
        printf("\n");                                                                                                  \
    }

#define BENCHMARK_SHA256X8_ALL BENCHMARK_SHA256X8(sha256x8_optim)
