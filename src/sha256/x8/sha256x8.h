#pragma once

#include "../types.h"
#include <stdint.h>

// can figure out how to set proper alignment requirements for compiler
// pointers need 16 byte alignment for CPU not to crash
// TODO :: debug wrapper to catch non aligned address in debug build, newer CPUs
// don't crash
void sha256x8_optim(uint8_t hash[SHA256_DIGEST_LENGTH * 8],
                    const uint8_t data[SHA256_INPUT_LENGTH * 8]);