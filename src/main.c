#include "hash.h"
#include "server/server.h"
#include "sha256/sha256.h"
#include "sha256/x4/sha256x4.h"
#include "sha256/x4x2/sha256x4x2.h"
#include "sha256/x8/sha256x8.h"
#include <stdint.h>
#include <stdio.h>

#include "benchmark.h"

int server(const int port) {
    int ret = 0;
    Server server;
    Client client;

    if ((ret = server_init(&server, port)) < 0) {
	printf("Failed to initialize server: %s\n", strerror(-ret));
	return 1;
    }

    if ((ret = server_listen(&server, 512)) < 0) {
	printf("Failed to listen on port %d: %s\n", port, strerror(-ret));
	return 1;
    }

    printf("Listening on port 8080\n");

    AsyncCtx ctx;

    if ((ret = async_server_init(&server, &ctx, &client)) < 0) {
	printf("Failed to initialize async server: %s\n", strerror(-ret));
	return 1;
    }

    while (1) {
	if ((ret = async_server_poll(&server, &ctx, &client)) >= 0) {
	    continue;
	}

	printf("Failed to poll server: %s\n", strerror(-ret));
	break;
    }

    async_server_exit(&server, &ctx, &client);

    printf("Closed server\n");

    return 0;
}

int benchmark() {
    BENCHMARK_SHA256_ALL
    // BENCHMARK_SHA256X8_ALL
    BENCHMARK_SHA256X4_ALL
    BENCHMARK_SHA256X4X2_ALL

    // BENCHMARK_REVERSE_HASH_ALL(0, 30000001, 30000000)

    // BENCHMARK_SHA256X4(sha256x4_cyclic)
    // printf("warmup done\n");

    // BENCHMARK_SHA256(sha256_optim)
    // BENCHMARK_SHA256X4(sha256x4_optim)
    // BENCHMARK_SHA256X4X2(sha256x4x2_optim)
    // BENCHMARK_SHA256X8(sha256x8_optim)

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
