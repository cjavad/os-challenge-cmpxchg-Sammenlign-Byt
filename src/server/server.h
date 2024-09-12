#pragma once

#include "../protocol.h"
#include "../thread/worker.h"
#include <errno.h>
#include <liburing.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

typedef struct sockaddr netinet_socketaddr;
typedef struct sockaddr_in netinet_socketaddr_in;

typedef struct {
    int fd;
    netinet_socketaddr_in addr;
} Server;

typedef struct {
    int fd;
    netinet_socketaddr_in addr;
} Client;

// Setup socket fd (shared between all implementations)
int server_init(Server *server, int port);
int server_listen(Server *server, int backlog);
int server_close(const Server *server);

// Simple server implementation.
int sync_server_init(Server *server, Client *client);
int sync_server_poll(Server *server, Client *client);
int sync_server_exit(const Server *server, Client *client);

// IO_URING context.
#define IOURING_BUFFER_SIZE 4096
#define IOURING_BUFFER_COUNT 4096
#define IOURING_ENTRIES 2048

typedef struct io_uring AsyncCtx;

// Pack all the information we need into an uint64_t
// to prevent allocating once per SQE.
typedef struct {
    uint32_t fd;
    uint16_t type;
    uint16_t bid;
} AsyncData;

_Static_assert(sizeof(AsyncData) <= sizeof(uint64_t),
               "UserData struct is too large");

// Enum over the different types of async operations.
typedef enum {
    ACCEPT,
    READ,
    WRITE,
    PROV_BUF,
    FUTEX,
} AsyncOperation;

int async_server_init(const Server *server, AsyncCtx *ctx, Client *client);
int async_server_poll(const Server *server, AsyncCtx *ctx, Client *client);
int async_server_exit(const Server *server, AsyncCtx *ctx, Client *client);