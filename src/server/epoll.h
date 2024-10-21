#include "server.h"

#include <errno.h>
#include <stdio.h>
#include <sys/epoll.h>

#define EPOLL_MAX_EVENTS 64

struct EpollServerCtx {
    WorkerPool* worker_pool;
    Scheduler* scheduler;
    int epoll_fd;
};

enum EpollServerEventType {
    SERVER_ACCEPT,
    CLIENT_EVENT,
};

union EpollEventData {
    struct {
        enum EpollServerEventType type;
        int32_t fd;
    };

    uint64_t u64;
};

// Both io_uring and epoll allow for 64 bits of user data per request.
_Static_assert(sizeof(union EpollEventData) <= sizeof(uint64_t), "UserData struct is too large");

int epoll_server_init(const Server* server, struct EpollServerCtx* ctx);
int epoll_server_poll(const struct EpollServerCtx* ctx);
int epoll_server_exit(const Server* server, const struct EpollServerCtx* ctx);
