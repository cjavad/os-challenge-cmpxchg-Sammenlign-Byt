#pragma once

#include "sha256/sha256.h"
#include <stdint.h>

uint64_t reverse_hash(uint64_t start, uint64_t end, HashDigest target);