#pragma once

#include "types.h"
#include "x1/sha256x1.h"
#include "x4/sha256x4.h"
#include "x4x2/sha256x4x2.h"
#include "x8/sha256x8.h"
#include <stdint.h>
#include <stdio.h>

void sprintf_hash(char* str, const HashDigest hash);
void print_hash(const HashDigest hash);

uint64_t reverse_sha256(uint64_t start, uint64_t end, const HashDigest target);
uint64_t reverse_sha256_x4(uint64_t start, uint64_t end, const HashDigest target);