#pragma once

#include "epoll.h"
#include "simple.h"

#define server_impl_init(ctx) \
    _Generic((ctx), \
        struct EpollServerCtx*: epoll_server_init, \
        struct SimpleServerCtx*: simple_server_init \
    )(ctx)

#define server_impl_poll(ctx) \
    _Generic((ctx), \
        struct EpollServerCtx*: epoll_server_poll, \
        struct SimpleServerCtx*: simple_server_poll \
    )(ctx)

#define server_impl_exit(ctx) \
    _Generic((ctx), \
        struct EpollServerCtx*: epoll_server_exit, \
        struct SimpleServerCtx*: simple_server_exit \
    )(ctx)
