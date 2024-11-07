#pragma once

#include <linux/futex.h>
#include <stdint.h>
#include <sys/time.h>

int64_t futex(uint32_t* uaddr, int futex_op, int val, const struct timespec* timeout, uint32_t* uaddr2, int val3);

int64_t futex_wait(uint32_t* uaddr);
int64_t futex_wake_single(uint32_t* uaddr);