#pragma once

#include "../sha256.h"
#include <stdint.h>

// can figure out how to set proper alignment requirements for compiler
// pointers need 16 byte alignment for CPU not to crash
// TODO :: debug wrapper to catch non aligned address in debug build, newer CPUs
// don't crash
void sha256x8_optim(uint8_t hash[SHA256_DIGEST_LENGTH * 8],
                    const uint8_t data[SHA256_INPUT_LENGTH * 8]);

#define BENCHMARK_SHA256X8(func)                                               \
    {                                                                          \
        uint8_t hash[SHA256_DIGEST_LENGTH * 8] __attribute__((aligned(16)));   \
        BENCHMARK_START                                                        \
        for (uint64_t i = 0; i < SHA256_BENCHMARK_ITERATIONS; i += 8) {        \
            uint64_t data[8]                                                   \
                __attribute__((aligned(16))) = {i, i + 1, i + 2, i + 3, i + 4, i + 5, i + 6, i + 7}; \
            (func)(hash, (uint8_t *)data);                                     \
        }                                                                      \
        BENCHMARK_END(func, SHA256_BENCHMARK_ITERATIONS)                       \
        printf("Hash: ");                                                      \
        print_hash(&hash[SHA256_DIGEST_LENGTH * 7]);                           \
        printf("\n");                                                          \
    }

#define BENCHMARK_SHA256X8_ALL \
	BENCHMARK_SHA256X8(sha256x8_optim)