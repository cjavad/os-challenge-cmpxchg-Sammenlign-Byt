#include "server.h"

// epoll server implementation.
#if !USE_IO_URING

int async_server_init(const Server* server, AsyncCtx* ctx, Client* client) {
    int ret = 0;

    bzero(ctx, sizeof(AsyncCtx));

    ret = epoll_create(EPOLL_MAX_EVENTS);

    if (ret < 0) {
	return ret;
    }

    // Setup server fd.
    ctx->epoll_fd = ret;
    ctx->ev.events = EPOLLIN;

    async_data_pack((AsyncData*)&ctx->ev.data.u64, server->fd, 0, ACCEPT);

    if ((ret = epoll_ctl(ctx->epoll_fd, EPOLL_CTL_ADD, server->fd, &ctx->ev)) <
        0) {
	return ret;
    }

    // Setup waker fd.
    ret = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);

    if (ret < 0) {
	return ret;
    }

    return ret;
}

int async_server_poll(const Server* server, AsyncCtx* ctx, Client* client) {
    int ret = 0;
    int new_events =
        epoll_wait(ctx->epoll_fd, ctx->events, EPOLL_MAX_EVENTS, -1);

    if (new_events < 0) {
	return new_events;
    }

    for (int i = 0; i < new_events; i++) {
	int bytes_received;
	const struct epoll_event* event = &ctx->events[i];
	WorkerState* state = NULL;
	AsyncOperation type;
	int32_t fd1;
	int32_t bid;

	async_data_unpack((AsyncData*)&event->data.u64, &fd1, &bid, &type);

        // Temp workaround.
	if (type == READ && event->events & EPOLLHUP) {
	    type = CLOSE;
	}

	switch (type) {

	// Accept new connection.
	case ACCEPT:
	    unsigned int client_len = sizeof(client->addr);

	    fd1 = accept4(
	        fd1, (struct sockaddr*)&client->addr, &client_len, SOCK_NONBLOCK
	    );

	    if (fd1 < 0) {
		fprintf(
		    stderr, "Failed to set accept client: %s\n", strerror(fd1)
		);

		continue;
	    }

	    // Handle READ and CLOSE events.
	    ctx->ev.events = EPOLLIN | EPOLLHUP;

	    async_data_pack((AsyncData*)&ctx->ev.data.u64, fd1, 0, READ);

	    if ((ret = epoll_ctl(ctx->epoll_fd, EPOLL_CTL_ADD, fd1, &ctx->ev)) <
	        0) {

		fprintf(
		    stderr, "Failed to add client %d to epoll: %s\n", fd1,
		    strerror(ret)
		);

		goto close;
	    }

	    break;
	// Read from connection
	case READ:
	    state = worker_create_shared_state();

	    // Store client fd.
	    state->data.fd = fd1;

	    bytes_received = recv(fd1, &state->request, PROTOCOL_REQ_SIZE, 0);

	    if (bytes_received == 0) {
		fprintf(stderr, "Received 0 bytes\n");
		break;
	    }

	    if (bytes_received < 0) {
		fprintf(
		    stderr, "Failed to read from client: %s\n",
		    strerror(bytes_received)
		);
		goto free_state;
	    }

	    if (bytes_received != PROTOCOL_REQ_SIZE) {
		fprintf(stderr, "Invalid request size: %d\n", bytes_received);
		goto free_state;
	    }

	    protocol_request_to_le(&state->request);

	    // protocol_debug_print_request(&state->request);

	    if (bid < 0) {
		fprintf(stderr, "Failed to store state\n");
		goto free_state;
	    }

	    spawn_worker_thread(state);

	    break;
	case CLOSE:
	    goto deregister;
	free_state:
	    if (state != NULL) {
		fprintf(stderr, "Freeing state: %p\n", state);
		worker_destroy_shared_state(state);
	    }
	deregister:
	    epoll_ctl(ctx->epoll_fd, EPOLL_CTL_DEL, fd1, NULL);
	close:
	    close(fd1);
	    break;
	default:
	    fprintf(
	        stderr, "Unknown event: %d type: %d\n", event->events, type
	    );
	    break;
	}
    }

    return 0;
}

int async_server_exit(const Server* server, AsyncCtx* ctx, Client* client) {
    return close(ctx->epoll_fd) + close(server->fd);
}

#endif