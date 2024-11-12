#pragma once


#include <stdint.h>
#include "atomic.h"

/*
    Spinning read-write spinlock implementation.

        Fits in 8-bytes.
*/

struct SRWLock
{
    Atomic(uint32_t) lock;
};

#define SPIN_RWLOCK_WRITER_MASK 0x80000000 // Highest bit for writer lock
#define SPIN_RWLOCK_READER_MASK 0x7FFFFFFF // Remaining bits for reader count

void spin_rwlock_init(struct SRWLock* lock);
void spin_rwlock_rdlock(struct SRWLock* lock);
void spin_rwlock_rdunlock(struct SRWLock* lock);
void spin_rwlock_wrlock(struct SRWLock* lock);
void spin_rwlock_wrunlock(struct SRWLock* lock);
