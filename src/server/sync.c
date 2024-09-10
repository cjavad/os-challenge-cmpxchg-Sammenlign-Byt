#include "server.h"

// Simple server implementation.
int sync_server_init(Server *server, Client *client) { return 0; }

int sync_server_poll(Server *server, Client *client) {
    bzero(client, sizeof(Client));

    uint32_t len = sizeof(client->addr);
    client->fd = accept(server->fd, (netinet_socketaddr *)&client->addr, &len);

    if (client->fd < 0) {
        return -1;
    }

    // Read struct BEProtocolRequest
    ProtocolRequest req;

    if (read(client->fd, &req, sizeof(ProtocolRequest)) !=
        sizeof(ProtocolRequest)) {
        return -1;
    }

    protocol_request_to_le(&req);

    protocol_debug_print_request(&req);

    ProtocolResponse resp;

    bzero(&resp, sizeof(ProtocolResponse));

    resp.answer = 0x69;

    protocol_debug_print_response(&resp);

    if (write(client->fd, &resp, sizeof(ProtocolResponse)) !=
        sizeof(ProtocolResponse)) {
        return -1;
    }

    if (close(client->fd) < 0) {
        return -1;
    }

    return 0;
}

int sync_server_exit(const Server *server, Client *client) {
    return server_close(server);
}
