#pragma once

#include "../thread/worker.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/version.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
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

#include <sys/epoll.h>
#include <sys/eventfd.h>

#define EPOLL_MAX_EVENTS 64

typedef struct AsyncCtx {
    int epoll_fd;
    WorkerPool* worker_pool;
    Scheduler* scheduler;
    struct epoll_event ev;
    struct epoll_event events[EPOLL_MAX_EVENTS];
} AsyncCtx;

typedef enum AsyncOperation {
    SERVER_ACCEPT,
    CLIENT_EVENT,
} AsyncOperation;

typedef struct AsyncData {
    union {
        struct {
            AsyncOperation type;
            int32_t fd;
        };

        uint64_t u64;
    };
} AsyncData;

// Both io_uring and epoll allow for 64 bits of user data per request.
_Static_assert(sizeof(AsyncData) <= sizeof(uint64_t), "UserData struct is too large");

int async_server_init(const Server* server, AsyncCtx* ctx);
int async_server_poll(const Server* server, AsyncCtx* ctx);
int async_server_exit(const Server* server, const AsyncCtx* ctx);
