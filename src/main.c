#include "hash.h"
#include "server/server.h"

#include <stdint.h>
#include <stdio.h>

#include "benchmark.h"

int server(const int port) {
    Server server;
    AsyncCtx ctx;
    int ret = 0;

    if ((ret = server_init(&server, port)) < 0) {
        printf("Failed to initialize server: %s\n", strerror(-ret));
        return 1;
    }

    if ((ret = server_listen(&server, 512)) < 0) {
        printf("Failed to listen on port %d: %s\n", port, strerror(-ret));
        return 1;
    }

    printf("Listening on port 8080\n");

    if ((ret = async_server_init(&server, &ctx)) < 0) {
        printf("Failed to initialize async server: %s\n", strerror(-ret));
        return 1;
    }

    while (1) {
        if ((ret = async_server_poll(&server, &ctx)) >= 0) {
            continue;
        }

        printf("Failed to poll server: %s\n", strerror(-ret));
        break;
    }

    async_server_exit(&server, &ctx);

    printf("Closed server\n");

    return 0;
}

int benchmark() {
    // benchmark_hash();
    benchmark_scheduler();
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s benchmark|<port>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "benchmark") == 0) {
        return benchmark();
    }

    const int port = atoi(argv[1]);

    return server(port);
}
