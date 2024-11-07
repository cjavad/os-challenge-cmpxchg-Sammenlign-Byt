#pragma once

#include <netinet/in.h>

typedef struct {
    int fd;
    struct sockaddr_in addr;
} Server;

// Setup socket fd (shared between all implementations)
int server_init(Server* server, int port);
int server_listen(Server* server, int backlog);
int server_close(const Server* server);