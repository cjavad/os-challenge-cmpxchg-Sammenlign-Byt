#include "protocol.h"

#include "sha256/sha256.h"
#include <stdio.h>

inline void protocol_request_to_le(
    struct ProtocolRequest* request
) {
    request->start = __builtin_bswap64(request->start);
    request->end = __builtin_bswap64(request->end);
    sha256_normalize(request->hash);
}

inline void protocol_response_to_be(
    struct ProtocolResponse* response
) {
    response->answer = __builtin_bswap64(response->answer);
}

void protocol_debug_print_request(
    const struct ProtocolRequest* request
) {
    printf("ProtocolRequest(\n    hash=");
    print_hash(request->hash);
    printf(
        "    start=%lu\n    end=%lu\n    prio=%d\n)\n",
        request->start,
        request->end,
        request->priority
    );
}

void protocol_debug_print_response(
    const struct ProtocolResponse* response
) {
    printf("ProtocolResponse(answer=%lu)\n", response->answer);
}