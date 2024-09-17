#pragma once

#include "sha256/sha256.h"
#include "sha256/x4/sha256x4.h"
#include <stdint.h>
#include <string.h>

uint64_t reverse_hash(uint64_t start, uint64_t end, HashDigest target);
uint64_t reverse_hash_x4(uint64_t start, uint64_t end, HashDigest target);

#define BENCHMARK_REVERSE_HASH(func, _start, _end, target_uint64)              \
    {                                                                          \
        uint64_t target_result = target_uint64;                                \
        HashDigest target;                                                     \
        sha256_openssl(target, (uint8_t *)&target_result);                     \
        BENCHMARK_START                                                        \
        uint64_t res = func(_start, _end, target);                             \
        BENCHMARK_END(func, 1)                                                 \
        printf("Target: %lu, Result: %lu\n", target_result, res);              \
    }

#define BENCHMARK_REVERSE_HASH_ALL(start, end, target_uint64)                  \
    BENCHMARK_REVERSE_HASH(reverse_hash, start, end, target_uint64)            \
    BENCHMARK_REVERSE_HASH(reverse_hash_x4, start, end, target_uint64)