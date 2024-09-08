#pragma once

#include <stdint.h>

#define SHA256_DIGEST_LENGTH 32

typedef uint8_t HashDigest[SHA256_DIGEST_LENGTH];
typedef uint8_t HashInput[8];


void sha256_openssl(HashDigest hash, const HashInput data);
void sha256_custom(HashDigest hash, const HashInput data);

void sprintf_hash(char *str, const HashDigest hash);
void print_hash(const HashDigest hash);