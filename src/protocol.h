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

typedef struct ProtocolRequest ProtocolRequest;

struct ProtocolResponse {
    uint64_t answer;
};

typedef struct ProtocolResponse ProtocolResponse;

void protocol_request_to_le(ProtocolRequest* be);
void protocol_response_to_be(ProtocolResponse* le);

void protocol_debug_print_request(const ProtocolRequest* req);
void protocol_debug_print_response(const ProtocolResponse* resp);
