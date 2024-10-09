#pragma once

#include <errno.h>
#include <linux/futex.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/time.h>
#include <syscall.h>
#include <unistd.h>

int futex(uint32_t* uaddr, int futex_op, int val, const struct timespec* timeout, uint32_t* uaddr2, int val3);

void futex_wait(uint32_t* futex_ptr);
void futex_post(uint32_t* futex_ptr);