#pragma once

#include <stdint.h>

uint32_t xorshift32_prng_next(uint32_t* state);
uint64_t xorshift64_prng_next(uint64_t* state);