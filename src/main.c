#include <stdio.h>
#include "hash.h"
#include "protocol.h"

#include <string.h>
#include "server.h"

int main(int argc, char** argv) {
  SyncServer server;
  SyncServerConn conn;

  hash_u test;

  uint8_t data[] = {0x00};

  int x = sha256_hash(data, 1, test);

  printf("SHA256 hash: ");
  debug_print_hash(test);
  printf("\n len: %d\n", x);

  if (sync_server_init(&server, 8080) != 0) {
    printf("Failed to initialize server\n");
    return 1;
  }

  if (sync_server_listen(&server) != 0) {
    printf("Failed to listen on port 8080\n");
    return 1;
  }

  printf("Listening on port 8080\n");

  for (;;) {
    if (sync_server_conn_accept(&server, &conn) != 0) {
      printf("Failed to accept connection\n");
      break;
    }

    printf("Accepted connection from %d\n", conn.addr.sin_addr.s_addr);

    // Read struct BEProtocolRequest
    ProtocolRequest req;

    const size_t req_size = sizeof(ProtocolRequest);

    if (sync_server_conn_read(&conn, &req, req_size) != req_size) {
      printf("Failed to read request with size %zu\n", req_size);
      continue;
    }

    protocol_request_to_le(&req);

    printf("Got request with hash: ");
    debug_print_hash(req.hash);
    printf("\n Start: %lu, End: %lu\n", req.start, req.end);

    const size_t resp_size = sizeof(ProtocolResponse);
    ProtocolResponse resp;

    bzero(&resp, resp_size);

    protocol_response_to_be(&resp);

    if (sync_server_conn_write(&conn, &resp, resp_size) != resp_size) {
      printf("Failed to write response with size %zu\n", resp_size);
      continue;
    }

    if (sync_server_conn_close(&conn) != 0) {
      printf("Failed to close connection\n");
      continue;
    }

    printf("Closed connection\n");
  }

  sync_server_close(&server);

  printf("Closed server\n");
}
