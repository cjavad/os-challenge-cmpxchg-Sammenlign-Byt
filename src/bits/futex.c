#include "futex.h"

#include <errno.h>
#include <stdbool.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <xmmintrin.h>

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

#ifndef _BITS_FUTEX_SPIN_LIMIT
#define _BITS_FUTEX_SPIN_LIMIT 100
#endif


int64_t futex_wait(uint32_t* uaddr) {
#ifdef _BITS_FUTEX_TIMEOUT_NS
    const struct timespec timeout = {.tv_sec = 0, .tv_nsec = _BITS_FUTEX_TIMEOUT_NS};
#endif
#if _BITS_FUTEX_SPIN_LIMIT > 0
    int32_t spins;

#endif
start:
#if _BITS_FUTEX_SPIN_LIMIT > 0
    spins = 0;
spin:
#endif

    if (*uaddr != 0) {
        return 0;
    }

#if _BITS_FUTEX_SPIN_LIMIT > 0
    if (++spins < _BITS_FUTEX_SPIN_LIMIT) {
        _mm_pause();
        goto spin;
    }
#endif

    // If still zero after spin-wait, perform futex wait
    futex(
        uaddr,
        FUTEX_WAIT,
        0,
#ifdef _BITS_FUTEX_TIMEOUT_NS
        &timeout,
#else
        NULL,
#endif
        NULL,
        0
    );
    goto start;
}

int64_t futex_wake(uint32_t* uaddr) {
    // Set the value to 1 to signal all waiters
    *uaddr = 1;

    // Wake up all threads waiting on uaddr at once
    return futex(uaddr, FUTEX_WAKE, INT32_MAX, NULL, NULL, 0);
}