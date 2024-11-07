#include "server.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int server_init(
    Server* server,
    const int port
) {
    memset(server, 0, sizeof(Server));

    server->fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    if (server->fd < 0) {
        fprintf(stderr, "Failed to create socket: %s\n", strerror(errno));
        return -1;
    }

    const int opt_level = 1;

    if (setsockopt(
            server->fd,
            SOL_SOCKET,
            SO_REUSEADDR,
            &opt_level,
            sizeof(opt_level)
        ) < 0) {
        fprintf(stderr, "Failed to set socket options: %s\n", strerror(errno));
        return -1;
    }

    server->addr.sin_family = AF_INET;
    server->addr.sin_port = htons(port);
    server->addr.sin_addr.s_addr = INADDR_ANY;

    return 0;
}

int server_listen(
    Server* server,
    const int backlog
) {
    if (bind(
            server->fd,
            (struct sockaddr*)&server->addr,
            sizeof(struct sockaddr)
        ) < 0) {
        fprintf(stderr, "Failed to bin server to port: %s\n", strerror(errno));
        return -1;
    }

    if (listen(server->fd, backlog) < 0) {
        fprintf(stderr,
                "Failed to have server listen on port: %s\n",
                strerror(errno));
        return -1;
    }

    return 0;
}

int server_close(
    const Server* server
) {
    return close(server->fd);
}