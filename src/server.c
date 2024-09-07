#include "server.h"

#include <strings.h>

int sync_server_init(SyncServer* server, const int port) {
  bzero(server, sizeof(SyncServer));
  server->fd = socket(AF_INET, SOCK_STREAM, 0);

  if (server->fd < 0) {
    return -1;
  }

  if (setsockopt(server->fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) != 0) {
    return -1;
  }

  server->addr.sin_family = AF_INET;
  server->addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server->addr.sin_port = htons(port);

  return 0;
}

int sync_server_listen(SyncServer* server) {
  if (bind(server->fd, (netinet_socketaddr*)&server->addr, sizeof(server->addr)) != 0) {
    return -1;
  }

  if (listen(server->fd, 5) != 0) {
    return -1;
  }

  return 0;
}



int sync_server_close(const SyncServer* server) {
  return close(server->fd);
}

int sync_server_conn_accept(const SyncServer* server, SyncServerConn* conn) {
  bzero(conn, sizeof(SyncServerConn));

  uint32_t len = sizeof(conn->addr);
  conn->fd = accept(server->fd, (netinet_socketaddr*)&conn->addr, &len);

  if (conn->fd < 0) {
    return -1;
  }

  return 0;
}

int sync_server_conn_read(SyncServerConn* conn, void* buf, size_t len) {
  return read(conn->fd, buf, len);
}

int sync_server_conn_write(SyncServerConn* conn, void* buf, size_t len) {
  return write(conn->fd, buf, len);
}

int sync_server_conn_close(const SyncServerConn* conn) {
  return close(conn->fd);
}

