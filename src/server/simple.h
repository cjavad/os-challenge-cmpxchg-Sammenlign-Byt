#include "server.h"

struct SimpleServerCtx
{
    struct Server server;
    struct ServerScheduler sched;
};

// Simple server implementation.
int simple_server_init(struct SimpleServerCtx* ctx);
int simple_server_poll(struct SimpleServerCtx* ctx);
int simple_server_exit(struct SimpleServerCtx* ctx);
