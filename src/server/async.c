#include "server.h"

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <liburing.h>

int IOURING_GROUP_ID = 0;
uint8_t IO_URING_BUFFERS[IOURING_BUFFER_COUNT][IOURING_BUFFER_SIZE] = {0};

void add_accept(struct io_uring *ring, int fd, struct sockaddr *client_addr,
                socklen_t *client_len, unsigned flags);
void add_socket_read(struct io_uring *ring, int fd, unsigned gid, size_t size,
                     unsigned flags);
void add_socket_write(struct io_uring *ring, int fd, uint16_t bid, size_t size,
                      unsigned flags);
void add_provide_buf(struct io_uring *ring, uint16_t bid, unsigned gid);

int async_server_init(const Server *server, AsyncCtx *ctx, Client *client) {
    int ret = 0;

    struct io_uring_params params;

    bzero(ctx, sizeof(*ctx));
    bzero(&params, sizeof(params));

    if ((ret = io_uring_queue_init_params(2048, ctx, &params)) < 0) {
        fprintf(stderr, "Failed to initialize io_uring params\n");
        return ret;
    }

    // check if IORING_FEAT_FAST_POLL is supported
    if (!(params.features & IORING_FEAT_FAST_POLL)) {
        fprintf(stderr, "IORING_FEAT_FAST_POLL not supported\n");
        return -1;
    }

    // check if buffer selection is supported
    struct io_uring_probe *probe = io_uring_get_probe_ring(ctx);

    if (!probe ||
        !io_uring_opcode_supported(probe, IORING_OP_PROVIDE_BUFFERS)) {
        fprintf(stderr, "IORING_OP_PROVIDE_BUFFERS not supported\n");

        return -1;
    }

    io_uring_free_probe(probe);

    // register buffers for buffer selection
    struct io_uring_sqe *sqe = io_uring_get_sqe(ctx);

    io_uring_prep_provide_buffers(sqe, IO_URING_BUFFERS, IOURING_BUFFER_SIZE,
                                  IOURING_BUFFER_COUNT, IOURING_GROUP_ID, 0);

    if ((ret = io_uring_submit(ctx)) < 0) {
        fprintf(stderr, "Failed to submit provide buffers\n");
        return ret;
    }

    struct io_uring_cqe *cqe;

    if ((ret = io_uring_wait_cqe(ctx, &cqe)) < 0) {
        fprintf(stderr, "Failed to wait for provide buffers cqe\n");
        return ret;
    }

    if ((ret = cqe->res) < 0) {
        fprintf(stderr, "Failed to provide buffers\n");
        return ret;
    }

    io_uring_cqe_seen(ctx, cqe);

    // add first accept SQE to monitor for new incoming connections
    socklen_t client_len = sizeof(client->addr);
    add_accept(ctx, server->fd, (netinet_socketaddr *)&client->addr,
               &client_len, 0);

    return ret;
}

int async_server_poll(const Server *server, AsyncCtx *ctx, Client *client) {
    io_uring_submit_and_wait(ctx, 1);

    struct io_uring_cqe *cqe;

    unsigned head;
    unsigned count = 0;
    socklen_t client_len = sizeof(client->addr);

    io_uring_for_each_cqe(ctx, head, cqe) {
        ++count;
        AsyncData data;
        memcpy(&data, &cqe->user_data, sizeof(data));
        const AsyncOperation type = data.type;

        // Invalid operation.
        if (cqe->res < 0) {
            return cqe->res;
        }

        switch (type) {

        case ACCEPT:
            const int sock_conn_fd = cqe->res;
            // only read when there is no error, >= 0
            if (sock_conn_fd >= 0) {
                add_socket_read(ctx, sock_conn_fd, IOURING_GROUP_ID,
                                PROTOCOL_REQ_SIZE, IOSQE_BUFFER_SELECT);
            }

            // new connected client; read data from socket and re-add accept
            // to monitor for new connections
            add_accept(ctx, server->fd, (netinet_socketaddr *)&client->addr,
                       &client_len, 0);
            break;
        case READ:
            const int bytes_read = cqe->res;
            const int bid = cqe->flags >> 16;

            if (cqe->res <= 0) {
                // read failed, re-add the buffer
                add_provide_buf(ctx, bid, IOURING_GROUP_ID);
                // connection closed or error
                close(data.fd);
            } else {
                // Parse request and generate response.
                ProtocolRequest req;
                memcpy(&req, IO_URING_BUFFERS[bid], PROTOCOL_REQ_SIZE);
                protocol_request_to_le(&req);

                protocol_debug_print_request(&req);

                ProtocolResponse resp;
                bzero(&resp, sizeof(ProtocolResponse));

                resp.answer = 0x1337;

                protocol_debug_print_response(&resp);

                protocol_response_to_be(&resp);

                // Copy response to buffer.
                memcpy(IO_URING_BUFFERS[bid], &resp, PROTOCOL_RES_SIZE);

                // bytes have been read into bufs, now add write to socket
                // sqe
                add_socket_write(ctx, data.fd, bid, PROTOCOL_RES_SIZE, 0);
            }
            break;
        case WRITE:
            // write has been completed, first re-add the buffer
            add_provide_buf(ctx, data.bid, IOURING_GROUP_ID);

            // Close connection after writing response.
            close(data.fd);
            break;
        case PROV_BUF:
            // Invalid buffer.
            if (cqe->res < 0) {
                return cqe->res;
            }

            break;
        }
    }

    io_uring_cq_advance(ctx, count);

    return 0;
}

int async_server_exit(const Server *server, AsyncCtx *ctx, Client *client) {
    io_uring_queue_exit(ctx);
    return server_close(server);
}

void add_accept(struct io_uring *ring, const int fd,
                struct sockaddr *client_addr, socklen_t *client_len,
                const unsigned flags) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_uring_prep_accept(sqe, fd, client_addr, client_len, 0);
    io_uring_sqe_set_flags(sqe, flags);

    const AsyncData data = {
        .fd = fd,
        .type = ACCEPT,
    };
    memcpy(&sqe->user_data, &data, sizeof(data));
}

void add_socket_read(struct io_uring *ring, const int fd, const unsigned gid,
                     const size_t size, const unsigned flags) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_uring_prep_recv(sqe, fd, NULL, size, 0);
    io_uring_sqe_set_flags(sqe, flags);
    sqe->buf_group = gid;

    const AsyncData data = {
        .fd = fd,
        .type = READ,
    };
    memcpy(&sqe->user_data, &data, sizeof(data));
}

void add_socket_write(struct io_uring *ring, const int fd, const uint16_t bid,
                      const size_t size, const unsigned flags) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_uring_prep_send(sqe, fd, &IO_URING_BUFFERS[bid], size, 0);
    io_uring_sqe_set_flags(sqe, flags);

    const AsyncData data = {
        .fd = fd,
        .type = WRITE,
        .bid = bid,
    };
    memcpy(&sqe->user_data, &data, sizeof(data));
}

void add_provide_buf(struct io_uring *ring, const uint16_t bid,
                     const unsigned gid) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_uring_prep_provide_buffers(sqe, IO_URING_BUFFERS[bid],
                                  IOURING_BUFFER_SIZE, 1, gid, bid);

    const AsyncData data = {
        .fd = 0,
        .type = PROV_BUF,
    };
    memcpy(&sqe->user_data, &data, sizeof(data));
}