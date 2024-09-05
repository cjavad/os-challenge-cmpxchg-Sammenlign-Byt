#include "protocol.h"

void protocol_request_to_le(ProtocolRequest* req) {
  req->start = htobe64(req->start);
  req->end = htobe64(req->end);

  for (int i = 0; i < 8; i++) {
    // req->hash.u32[i] = htobe32(req->hash.u32[i]);
  }
}

void protocol_response_to_be(ProtocolResponse* resp) {
  resp->answer = htobe64(resp->answer);
}