#include "protocol.h"

void protocol_request_to_le(ProtocolRequest *req) {
    req->start = ntole64(req->start);
    req->end = ntole64(req->end);
}

void protocol_response_to_be(ProtocolResponse *resp) {
    resp->answer = htobe64(resp->answer);
}
