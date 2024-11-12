#include "spin.h"
#include <immintrin.h>
#include <stdatomic.h>

inline void spin_rwlock_init(struct SRWLock* lock) {
    atomic_store(&lock->lock, 0);
}

inline void spin_rwlock_rdlock(struct SRWLock* lock) {
    while (1) {
        uint32_t e = atomic_load(&lock->lock) & SPIN_RWLOCK_READER_MASK;
        const uint32_t d = e + 1;

        if (atomic_compare_exchange_weak(&lock->lock, &e, d)) {
            break;
        }

        _mm_pause();
    }
}

inline void spin_rwlock_rdunlock(struct SRWLock* lock) {
    atomic_fetch_sub(&lock->lock, 1);
}

inline void spin_rwlock_wrlock(struct SRWLock* lock) {
    uint32_t e = 0;

    while (
        !atomic_compare_exchange_weak(&lock->lock, &e, SPIN_RWLOCK_WRITER_MASK)
    ) {
        e = 0;
        _mm_pause();
    }
}

inline void spin_rwlock_wrunlock(struct SRWLock* lock) {
    atomic_fetch_and(&lock->lock, SPIN_RWLOCK_READER_MASK);
}