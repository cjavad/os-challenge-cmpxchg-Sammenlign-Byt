#include "protocol.h"

#include "sha256/sha256.h"
#include <stdio.h>

inline void protocol_request_to_le(
    struct ProtocolRequest* req
) {
    req->start = __builtin_bswap64(req->start);
    req->end = __builtin_bswap64(req->end);
}

inline void protocol_response_to_be(
    struct ProtocolResponse* resp
) {
    resp->answer = __builtin_bswap64(resp->answer);
}

void protocol_debug_print_request(
    const struct ProtocolRequest* req
) {
    printf("ProtocolRequest(\n    hash=");
    print_hash(req->hash);
    printf(
        "    start=%lu\n    end=%lu\n    prio=%d\n)\n",
        req->start,
        req->end,
        req->priority
    );
}

void protocol_debug_print_response(
    const struct ProtocolResponse* resp
) {
    printf("ProtocolResponse(answer=%lu)\n", resp->answer);
}