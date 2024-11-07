#include "simple.h"
#include "../protocol.h"

#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// Simple server implementation.
int simple_server_init() { return 0; }

int simple_server_poll(const Server* server) {
    struct sockaddr_in addr = {0};
    uint32_t client_len = sizeof(addr);

    const int32_t client_fd = accept(server->fd, (struct sockaddr*)&addr, &client_len);

    if (client_fd < 0) {
        return -1;
    }

    // Read struct BEProtocolRequest
    struct ProtocolRequest req;

    if (read(client_fd, &req, sizeof(struct ProtocolRequest)) != sizeof(struct ProtocolRequest)) {
        return -1;
    }

    protocol_request_to_le(&req);

    protocol_debug_print_request(&req);

    struct ProtocolResponse resp = {0};

    resp.answer = 0x69;

    protocol_debug_print_response(&resp);

    if (write(client_fd, &resp, sizeof(struct ProtocolResponse)) != sizeof(struct ProtocolResponse)) {
        return -1;
    }

    if (close(client_fd) < 0) {
        return -1;
    }

    return 0;
}

int simple_server_exit(const Server* server) { return server_close(server); }
