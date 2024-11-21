#pragma once

#include "../types.h"
#include <stdint.h>

// can figure out how to set proper alignment requirements for compiler
// pointers need 16 byte alignment for CPU not to crash
// TODO :: debug wrapper to catch non aligned address in debug build, newer CPUs
// don't crash
void sha256x4_optim(
    uint8_t hash[SHA256_DIGEST_LENGTH * 4],
    const uint8_t data[SHA256_INPUT_LENGTH * 4]
);

void sha256x4_fused(
    uint8_t hash[SHA256_DIGEST_LENGTH * 4],
    const uint8_t data[SHA256_INPUT_LENGTH * 4]
);

void sha256x4_fullyfused(
    uint8_t hash[SHA256_DIGEST_LENGTH * 4],
    const uint8_t data[SHA256_INPUT_LENGTH * 4]
);

void sha256x4_cyclic(
    uint8_t hash[SHA256_DIGEST_LENGTH * 4],
    const uint8_t data[SHA256_INPUT_LENGTH * 4]
);

void sha256x4_asm(
    uint8_t hash[SHA256_DIGEST_LENGTH * 4],
    const uint8_t data[SHA256_INPUT_LENGTH * 4]
);

// Remember to call these with aligned data!

void sha256x4_cyclic_asm(
    uint8_t hash[SHA256_DIGEST_LENGTH * 4],
    const uint8_t data[SHA256_INPUT_LENGTH * 4]
);

void sha256x4_fullyfused_asm(
    uint8_t hash[SHA256_DIGEST_LENGTH * 4],
    const uint8_t data[SHA256_INPUT_LENGTH * 4]
);