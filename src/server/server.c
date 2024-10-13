#include <stdio.h>

#include "server.h"

int server_init(Server* server, const int port) {
    int ret = 0;

    bzero(server, sizeof(Server));
    ret = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    if (ret < 0) {
        fprintf(stderr, "Failed to create socket: %s\n", strerror(-ret));
        return ret;
    }

    server->fd = ret;

    const int opt_level = 1;

    printf("Got socket fd: %d\n", server->fd);

    if ((ret = setsockopt(server->fd, SOL_SOCKET, SO_REUSEADDR, &opt_level, sizeof(opt_level))) < 0) {

        fprintf(stderr, "Failed to set socket options: %s\n", strerror(-ret));
        return ret;
    }

    server->addr.sin_family = AF_INET;
    server->addr.sin_port = htons(port);
    server->addr.sin_addr.s_addr = INADDR_ANY;

    return ret;
}

int server_listen(Server* server, const int backlog) {
    int ret = 0;

    if ((ret = bind(server->fd, (struct sockaddr*)&server->addr, sizeof(struct sockaddr))) < 0) {
        return ret;
    }

    if ((ret = listen(server->fd, backlog)) < 0) {
        return ret;
    }

    return ret;
}

int server_close(const Server* server) { return close(server->fd); }
