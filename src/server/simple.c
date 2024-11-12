#include "simple.h"
#include "../protocol.h"
#include "../scheduler/generic.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// Simple server implementation.
int simple_server_init(struct SimpleServerCtx* ctx) {
    // Make socket blocking again
    if (fcntl(ctx->server.fd, F_SETFL, SOCK_STREAM) < 0) {
        return -1;
    }

    server_scheduler_init(&ctx->sched, 32);
    return 0;
}

int simple_server_poll(
    struct SimpleServerCtx* ctx
) {
    struct sockaddr_in addr = {0};
    uint32_t client_len = sizeof(addr);

    const int32_t client_fd =
        accept(ctx->server.fd, (struct sockaddr*)&addr, &client_len);

    if (client_fd < 0) {
        fprintf(stderr, "Failed to accept connection\n");
        return -1;
    }

    struct ProtocolRequest req;

    if (read(client_fd, &req, sizeof(struct ProtocolRequest)) !=
        PROTOCOL_REQ_SIZE) {
        return 0;
    }

    protocol_request_to_le(&req);

    struct SchedulerJobRecipient* recipient = scheduler_create_job_recipient(
        SCHEDULER_JOB_RECIPIENT_TYPE_FD,
        client_fd
    );

    scheduler_submit(ctx->sched.scheduler, &req, recipient);

    return 0;
}

int simple_server_exit(
    struct SimpleServerCtx* ctx
) {
    server_scheduler_destroy(&ctx->sched);
    return server_close(&ctx->server);
}