#pragma once

#include "../protocol.h"
#include "../thread/worker.h"
#include <errno.h>
#include <linux/version.h>
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
int server_init(Server* server, int port);
int server_listen(Server* server, int backlog);
int server_close(const Server* server);

// Simple server implementation.
int sync_server_init(Server* server, Client* client);
int sync_server_poll(Server* server, Client* client);
int sync_server_exit(const Server* server, Client* client);

#define USE_IO_URING 0

#if USE_IO_URING
#include <liburing.h>

// IO_URING context.
#define IOURING_BUFFER_SIZE 4096
#define IOURING_BUFFER_COUNT 4096
#define IOURING_ENTRIES 2048
typedef struct io_uring AsyncCtx;

// Pack all the information we need into an uint64_t
// to prevent allocating once per SQE.
typedef struct AsyncData {
    uint32_t fd;
    uint16_t type;
    uint16_t bid;
} AsyncData;

// Enum over the different types of async operations.
typedef enum AsyncOperation {
    ACCEPT,
    READ,
    WRITE,
    PROV_BUF,
    FUTEX,
} AsyncOperation;

#else
#include <sys/epoll.h>

#define EPOLL_MAX_EVENTS 64

typedef struct AsyncCtx {
    int epoll_fd;
    struct epoll_event ev;
    struct epoll_event events[EPOLL_MAX_EVENTS];
} AsyncCtx;

typedef struct AsyncData {
    uint64_t fd2 : 31;
    uint64_t fd1 : 31;
    uint64_t type : 2;
} AsyncData __attribute__((aligned(8)));

typedef enum AsyncOperation {
    ACCEPT,
    READ,
    EVENT_FD,
} AsyncOperation;

inline void async_data_pack(
    AsyncData* data, const int32_t fd1, const int32_t fd2,
    const AsyncOperation type
) {
    data->fd1 = fd1 & 0x7FFFFFFF;
    data->fd2 = fd2 & 0x7FFFFFFF;
    data->type = type & 0x3;
}

inline void async_data_unpack(
    const AsyncData* data, int32_t* fd1, int32_t* fd2, AsyncOperation* type
) {
    *fd1 = data->fd1;
    *fd2 = data->fd2;
    *type = data->type;
}

#endif

// Both io_uring and epoll allow for 64 bits of user data per request.
_Static_assert(
    sizeof(AsyncData) <= sizeof(uint64_t), "UserData struct is too large"
);

int async_server_init(const Server* server, AsyncCtx* ctx, Client* client);
int async_server_poll(const Server* server, AsyncCtx* ctx, Client* client);
int async_server_exit(const Server* server, AsyncCtx* ctx, Client* client);