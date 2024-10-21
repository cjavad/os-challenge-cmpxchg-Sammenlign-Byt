#include "futex.h"

int64_t futex(
    uint32_t* uaddr, const int futex_op, const int val, const struct timespec* timeout, uint32_t* uaddr2, const int val3
) {
    return syscall(SYS_futex, uaddr, futex_op, val, timeout, uaddr2, val3);
}

void futex_wait(uint32_t* futex_ptr) {
    while (1) {
        if (__sync_bool_compare_and_swap(futex_ptr, 1, 0))
            break;

        const int64_t s = futex(futex_ptr, FUTEX_WAIT, 0, NULL, NULL, 0);

        if (s == -1 && errno != EAGAIN)
            break;
    }
}

void futex_post(uint32_t* futex_ptr) {
    if (__sync_bool_compare_and_swap(futex_ptr, 0, 1)) {
        const int64_t s = futex(futex_ptr, FUTEX_WAKE, 1, NULL, NULL, 0);

        if (s == -1) {
        }
    }
}