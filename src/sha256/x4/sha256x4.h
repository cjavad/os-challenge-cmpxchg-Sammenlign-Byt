#pragma once

#include "../sha256.h"
#include <stdint.h>

// can figure out how to set proper alignment requirements for compiler
// pointers need 16 byte alignment for CPU not to crash
// TODO :: debug wrapper to catch non aligned address in debug build, newer CPUs
// don't crash
void sha256x4_optim(uint8_t hash[SHA256_DIGEST_LENGTH * 4],
                    const uint8_t data[SHA256_INPUT_LENGTH * 4]);

void sha256x4_fused(uint8_t hash[SHA256_DIGEST_LENGTH * 4],
					const uint8_t data[SHA256_INPUT_LENGTH * 4]);

void sha256x4_fullyfused(uint8_t hash[SHA256_DIGEST_LENGTH * 4],
					const uint8_t data[SHA256_INPUT_LENGTH * 4]);
					
void sha256x4_cyclic(uint8_t hash[SHA256_DIGEST_LENGTH * 4],
					const uint8_t data[SHA256_INPUT_LENGTH * 4]);

#define BENCHMARK_SHA256X4(func)                                               \
    {                                                                          \
        uint8_t hash[SHA256_DIGEST_LENGTH * 4] __attribute__((aligned(16)));   \
        BENCHMARK_START                                                        \
        for (uint64_t i = 0; i < SHA256_BENCHMARK_ITERATIONS; i += 4) {        \
            uint64_t data[4]                                                   \
                __attribute__((aligned(16))) = {i, i + 1, i + 2, i + 3};       \
            (func)(hash, (uint8_t *)data);                                     \
        }                                                                      \
        BENCHMARK_END(func, SHA256_BENCHMARK_ITERATIONS)                       \
        printf("Hash: ");                                                      \
        print_hash(&hash[96]);                                                 \
        printf("\n");                                                          \
    }

#define BENCHMARK_SHA256X4_ALL \
	BENCHMARK_SHA256X4(sha256x4_optim) \
	BENCHMARK_SHA256X4(sha256x4_fused) \
	BENCHMARK_SHA256X4(sha256x4_fullyfused) \
	BENCHMARK_SHA256X4(sha256x4_cyclic)