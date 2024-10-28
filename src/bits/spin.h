#pragma once

#include <immintrin.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

/*
    Spinning read-write spinlock implementation.

        Fits in 8-bytes.
*/

struct SRWLock {
    uint32_t lock;
};

#define SPIN_RWLOCK_WRITER_MASK 0x80000000 // Highest bit for writer lock
#define SPIN_RWLOCK_READER_MASK 0x7FFFFFFF // Remaining bits for reader count

inline void spin_rwlock_init(struct SRWLock* lock) { atomic_store(&lock->lock, 0); }

inline void spin_rwlock_rdlock(struct SRWLock* lock) {
    while (true) {
        const uint32_t e = atomic_load(&lock->lock) & SPIN_RWLOCK_READER_MASK;
        const uint32_t d = e + 1;

        if (atomic_compare_exchange_weak(&lock->lock, &e, d)) {
            break;
        }

        _mm_pause();
    }
}

inline void spin_rwlock_rdunlock(struct SRWLock* lock) { atomic_fetch_sub(&lock->lock, 1); }

inline void spin_rwlock_wrlock(struct SRWLock* lock) {
    uint32_t e = 0;

    while (!atomic_compare_exchange_weak(&lock->lock, &e, SPIN_RWLOCK_WRITER_MASK)) {
        e = 0;
        // _mm_pause();
    }
}

inline void spin_rwlock_wrunlock(struct SRWLock* lock) { atomic_fetch_and(&lock->lock, SPIN_RWLOCK_READER_MASK); }