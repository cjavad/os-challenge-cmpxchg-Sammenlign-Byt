#pragma once

#include "sha256/types.h"

#define PROTOCOL_REQ_SIZE 49
#define PROTOCOL_RES_SIZE 8

#include <stdint.h>

// The format over the wire is big endian but all values are generated in little
// endian
struct ProtocolRequest {
    // Hash to be reversed
    HashDigest hash;
    // Range of the input that the hash originates from.
    // 64-bit unsigned integers
    uint64_t start;
    uint64_t end;
    // Priority
    uint8_t priority;
};

struct ProtocolResponse {
    uint64_t answer;
};

void protocol_request_to_le(struct ProtocolRequest* request);
void protocol_response_to_be(struct ProtocolResponse* response);

void protocol_debug_print_request(const struct ProtocolRequest* request);
void protocol_debug_print_response(const struct ProtocolResponse* response);
