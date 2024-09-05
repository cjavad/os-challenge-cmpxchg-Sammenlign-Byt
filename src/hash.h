#pragma once

#include <stdint.h>
#include <stdio.h>

typedef uint8_t hash_t[32];


void debug_print_hash(const hash_t hash);

int sha256_hash(const uint8_t* data, size_t len, hash_t hash);
