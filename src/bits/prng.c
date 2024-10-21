#include "prng.h"

// XORShift prng
//
// https://www.jstatsoft.org/article/view/v008i14
// https://en.wikipedia.org/wiki/Xorshift

uint32_t xorshift32_prng_next(uint32_t* state) {
    *state ^= *state << 13;
    *state ^= *state >> 17;
    *state ^= *state << 5;
    return *state;
}

uint64_t xorshift64_prng_next(uint64_t* state) {
    *state ^= *state << 23;
    *state ^= *state >> 17;
    *state ^= *state << 26;
    return *state;
}