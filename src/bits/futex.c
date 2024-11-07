#include "futex.h"

#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

int64_t futex(
    uint32_t* uaddr,
    const int futex_op,
    const int val,
    const struct timespec* timeout,
    uint32_t* uaddr2,
    const int val3
) {
    return syscall(SYS_futex, uaddr, futex_op, val, timeout, uaddr2, val3);
}

int64_t futex_wait(
    uint32_t* uaddr
) {
    while (1) {
        // wait for futex to be 1, then make it 0.
        if (__sync_bool_compare_and_swap(uaddr, 1, 0)) {
            return 0;
        }

        const int64_t s = futex(uaddr, FUTEX_WAIT, 0, NULL, NULL, 0);

        if (s == -1) {
            if (errno == EAGAIN || errno == EINTR) {
                continue;
            }

            return -1;
        }
    }
}

int64_t futex_wake_single(
    uint32_t* uaddr
) {
    if (__sync_bool_compare_and_swap(uaddr, 0, 1)) {
        return futex(uaddr, FUTEX_WAKE, 1, NULL, NULL, 0);
    }

    // No need to wake up.
    return 0;
}