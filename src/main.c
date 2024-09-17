#include "hash.h"
#include "server/server.h"
#include "sha256/sha256.h"
#include "sha256/x4/sha256x4.h"
#include <stdint.h>
#include <stdio.h>

int main(int argc, char **argv) {

    // BENCHMARK_SHA256_ALL
    //     BENCHMARK_SHA256X4_ALL

    //  BENCHMARK_REVERSE_HASH_ALL(0, 30000001, 30000000)

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
