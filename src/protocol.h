#pragma once

#include <stdint.h>

#define __USE_MISC
#include <endian.h>
#undef __USE_MISC

#include "hash.h"

// The format over the wire is big endian but all values are generated in little
// endian

typedef struct {
    // Hash to be reversed
    hash_u hash;
    // Range of the input that the hash originates from.
    // 64-bit unsigned integers
    uint64_t start;
    uint64_t end;
    // Priority
    uint8_t p;
} __attribute__((packed)) ProtocolRequest;

typedef struct {
    uint64_t answer;
} __attribute__((packed)) ProtocolResponse;

void protocol_request_to_le(ProtocolRequest *be);
void protocol_response_to_be(ProtocolResponse *le);
