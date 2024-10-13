#include "server.h"

// Simple server implementation.
int simple_server_init(Server* server);
int simple_server_poll(const Server* server);
int simple_server_exit(const Server* server);