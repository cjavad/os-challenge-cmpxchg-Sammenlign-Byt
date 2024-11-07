#include "experiments/benchmark.h"
#include "experiments/misc.h"
#include "server/epoll.h"
#include "server/server.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

static volatile sig_atomic_t stop_flag = 0;

void signal_handler(
    const int signal
) {
    (void)signal;
    stop_flag = 1;
}

#ifdef PROFILE_GENERATION
#include <gcov.h>
#endif

int server(
    const int port
) {
    // Setup signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    Server server;
    struct EpollServerCtx ctx;

    if (server_init(&server, port) < 0) {
        fprintf(stderr, "Failed to initialize server: %s\n", strerror(errno));
        return 1;
    }

    if (server_listen(&server, 512) < 0) {
        fprintf(stderr,
                "Failed to listen on port %d: %s\n",
                port,
                strerror(errno));
        return 1;
    }

    fprintf(stderr, "Listening on port %i\n", port);

    if (epoll_server_init(&server, &ctx) < 0) {
        fprintf(stderr,
                "Failed to initialize async server: %s\n",
                strerror(errno));
        return 1;
    }

    while (!stop_flag) {
        if (epoll_server_poll(&ctx) >= 0) {
            continue;
        }

        fprintf(stderr, "Failed to poll server: %s\n", strerror(errno));
        break;
    }

    epoll_server_exit(&server, &ctx);

    fprintf(stderr, "Closed server\n");

#ifdef PROFILE_GENERATION
   __gcov_dump();
#endif

    return 0;
}

int benchmark() {
    benchmark_hash();
    benchmark_scheduler();
    benchmark_sha256_radix_tree_lookup();
    benchmark_random_key_radix_tree_lookup();
    benchmark_manual_radix_tree();
    return 0;
}

int main(
    int argc,
    char** argv
) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s benchmark|<port>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "benchmark") == 0) {
        return benchmark();
    }

    if (strcmp(argv[1], "misc") == 0) {
        return misc_main();
    }

    const int port = atoi(argv[1]);

    return server(port);
}