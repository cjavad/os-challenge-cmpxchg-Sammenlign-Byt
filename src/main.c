#include "experiments/benchmark.h"
#include "experiments/misc.h"
#include "server/server.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

int main(
    int argc,
    char** argv
) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s benchmark|misc|<port>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "benchmark") == 0) {
        benchmark();
        return 0;
    }

    if (strcmp(argv[1], "misc") == 0) {
        return misc_main();
    }

    const int port = atoi(argv[1]);

    server(port);
    return 0;
}