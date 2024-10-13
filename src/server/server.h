#pragma once

#include "../sha256/sha256.h"
#include "../thread/worker.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/version.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct {
    int fd;
    struct sockaddr_in addr;
} Server;

// Setup socket fd (shared between all implementations)
int server_init(Server* server, int port);
int server_listen(Server* server, int backlog);
int server_close(const Server* server);