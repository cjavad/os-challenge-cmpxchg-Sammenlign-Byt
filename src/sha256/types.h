#pragma once

#include <stdint.h>

#define SHA256_INPUT_LENGTH  8
#define SHA256_DIGEST_LENGTH 32

typedef uint8_t HashDigest[SHA256_DIGEST_LENGTH];
typedef uint8_t HashInput[SHA256_INPUT_LENGTH];

