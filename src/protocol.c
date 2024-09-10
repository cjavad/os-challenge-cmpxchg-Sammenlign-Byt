#include "protocol.h"

#include <stdio.h>

void protocol_request_to_le(ProtocolRequest *req) {
    req->start = htobe64(req->start);
    req->end = htobe64(req->end);
}

void protocol_response_to_be(ProtocolResponse *resp) {
    resp->answer = htobe64(resp->answer);
}
void protocol_debug_print_request(const ProtocolRequest *req) {
    printf("Got request with hash: ");
    print_hash(req->hash);
    printf("\nStart: %lu, End: %lu, Priority %d\n", req->start, req->end,
           req->priority);
}
void protocol_debug_print_response(const ProtocolResponse *resp) {
    printf("Answer: %lu\n", resp->answer);
}