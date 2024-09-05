#pragma once

#include <stdint.h>
#include <immintrin.h>

typedef struct {
	volatile uint8_t head;
	volatile uint8_t tail;
} SpinLock;

inline void spinlock_init(SpinLock* lock)
{
	lock->head = 0;
	lock->tail = 0;
}

inline void spinlock_aquire(SpinLock* lock)
{
	const uint32_t index = __atomic_fetch_add(&(lock->tail), 1, __ATOMIC_RELAXED);
	while (index != lock->tail) _mm_pause();
}

inline void spinlock_release(SpinLock* lock)
{
	lock->tail = lock->tail + 1;
}