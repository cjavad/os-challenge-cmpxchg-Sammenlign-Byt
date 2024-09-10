#pragma once

#define PROTOCOL_REQ_SIZE 49
#define PROTOCOL_RES_SIZE 8

#include <stdint.h>

#ifndef __USE_MISC
#define __USE_MISC
#include <endian.h>
#undef __USE_MISC
#else
#include <endian.h>
#endif

#include "hash.h"

// The format over the wire is big endian but all values are generated in little
// endian

typedef struct {
    // Hash to be reversed
    HashDigest hash;
    // Range of the input that the hash originates from.
    // 64-bit unsigned integers
    uint64_t start;
    uint64_t end;
    // Priority
    uint8_t priority;
} __attribute__((packed)) ProtocolRequest;

_Static_assert(sizeof(ProtocolRequest) == PROTOCOL_REQ_SIZE,
               "ProtocolRequest size is not 49 bytes");

typedef struct {
    uint64_t answer;
} __attribute__((packed)) ProtocolResponse;

_Static_assert(sizeof(ProtocolResponse) == PROTOCOL_RES_SIZE,
               "ProtocolResponse size is not 8 bytes");

void protocol_request_to_le(ProtocolRequest *be);
void protocol_response_to_be(ProtocolResponse *le);

void protocol_debug_print_request(const ProtocolRequest *req);
void protocol_debug_print_response(const ProtocolResponse *resp);