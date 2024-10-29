#include "futex.h"

int64_t futex(
    uint32_t* uaddr, const int futex_op, const int val, const struct timespec* timeout, uint32_t* uaddr2, const int val3
) {
    return syscall(SYS_futex, uaddr, futex_op, val, timeout, uaddr2, val3);
}