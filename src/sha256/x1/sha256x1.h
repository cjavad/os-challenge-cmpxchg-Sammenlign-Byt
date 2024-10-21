#pragma once

#include "../types.h"
#include <stdint.h>
#include <time.h>

void sha256_custom(HashDigest hash, const HashInput data);
void sha256_optim(HashDigest hash, const HashInput data);
void sha256_fused(HashDigest hash, const HashInput data);
void sha256_fullyfused(HashDigest hash, const HashInput data);
