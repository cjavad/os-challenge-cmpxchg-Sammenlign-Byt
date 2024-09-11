#include "hash.h"
#include "server/server.h"
#include <stdio.h>

int main(int argc, char **argv) {
    // return async_main();

    // BENCHMARK_SHA256_ALL

    HashDigest hash = {0x2d, 0xb8, 0xd6, 0xd0, 0x49, 0xb6, 0x1f, 0x55,
                       0xf0, 0x5a, 0xf9, 0x24, 0x1a, 0xf2, 0x45, 0x1f,
                       0x90, 0x6f, 0x1b, 0xed, 0xfd, 0xe2, 0x3d, 0xf,
                       0x75, 0xbd, 0x25, 0xce, 0x3f, 0xb8, 0xa4, 0x91};

    // BENCHMARK_START
    // uint64_t res = reverse_hash(0, 1610682842, hash);
    // BENCHMARK_END(reverse)
    // printf("Result: %lu\n", res);

    // return 0;

    int ret = 0;
    Server server;
    Client client;

    if ((ret = server_init(&server, 8080)) < 0) {
        printf("Failed to initialize server: %s\n", strerror(-ret));
        return 1;
    }

    if ((ret = server_listen(&server, 512)) < 0) {
        printf("Failed to listen on port 8080: %s\n", strerror(-ret));
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
