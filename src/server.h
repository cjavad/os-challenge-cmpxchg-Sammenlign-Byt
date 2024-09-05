#pragma once

#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> // read(), write(), close()
#include <netinet/in.h>

typedef struct sockaddr netinet_socketaddr;
typedef struct sockaddr_in netinet_socketaddr_in;

typedef struct {
  int fd;
  netinet_socketaddr_in addr;
} SyncServer;

typedef struct {
  int fd;
  netinet_socketaddr_in addr;
} SyncServerConn;

int sync_server_init(SyncServer* server, int port);
int sync_server_listen(SyncServer* server);
int sync_server_close(const SyncServer* server);

int sync_server_conn_accept(const SyncServer* server, SyncServerConn* conn);
int sync_server_conn_read(SyncServerConn* conn, void* buf, size_t len);
int sync_server_conn_write(SyncServerConn* conn, void* buf, size_t len);
int sync_server_conn_close(const SyncServerConn* conn);

