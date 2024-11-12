#include "misc.h"
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include "../bits/futex.h"
#include "../bits/atomic.h"

#include <errno.h>
#include <xmmintrin.h>

#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#define SLEEPING_THREADS 16
#define ITERATIONS 1000000

struct MutexCondWaker {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

struct FutexWaker {
    uint32_t futex;
};

struct SpinWaker {
    bool wake;
};

struct Waker {
    Atomic(uint32_t) started;
    Atomic(uint32_t) finished;

    union {
        struct MutexCondWaker mutex_cond_waker;
        struct FutexWaker futex_waker;
        struct SpinWaker spin_waker;
    };
};


void* mutex_cond_waker_thread(void* arg) {
    struct Waker* waker = arg;
    struct MutexCondWaker* mutex_cond_waker = &waker->mutex_cond_waker;

    pthread_mutex_lock(&mutex_cond_waker->mutex);
    atomic_fetch_add(&waker->started, 1);
    pthread_cond_wait(&mutex_cond_waker->cond, &mutex_cond_waker->mutex);
    atomic_fetch_add(&waker->finished, 1);
    pthread_mutex_unlock(&mutex_cond_waker->mutex);

    return NULL;
}

void* futex_waker_thread(void* arg) {
    struct Waker* waker = arg;

    atomic_fetch_add(&waker->started, 1);
    // Loop to handle spurious wakeups
    futex_wait(&waker->futex_waker.futex);
    atomic_fetch_add(&waker->finished, 1);

    return NULL;
}

void* spin_waker_thread(void* arg) {
    struct Waker* waker = arg;

    atomic_fetch_add(&waker->started, 1);

    while (!waker->spin_waker.wake)
        _mm_pause();

    atomic_fetch_add(&waker->finished, 1);

    return NULL;
}

void misc_pthread_waker_vs_pure_futex() {
    struct Waker* waker = calloc(1, sizeof(struct Waker));

    pthread_t threads[SLEEPING_THREADS];

    // First time the normal way
    atomic_store(&waker->started, 0);
    atomic_store(&waker->finished, 0);

    waker->mutex_cond_waker.mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    waker->mutex_cond_waker.cond = (pthread_cond_t)PTHREAD_COND_INITIALIZER;

    for (int i = 0; i < SLEEPING_THREADS; i++) {
        pthread_create(&threads[i],
                       NULL,
                       mutex_cond_waker_thread,
                       waker);
    }

    while (atomic_load(&waker->started) != SLEEPING_THREADS)
        _mm_pause();

    usleep(600000);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    pthread_cond_broadcast(&waker->mutex_cond_waker.cond);

    while (atomic_load(&waker->finished) != SLEEPING_THREADS)
        _mm_pause();

    clock_gettime(CLOCK_MONOTONIC, &end);

    for (int i = 0; i < SLEEPING_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("MutexCondWaker: %ld\n",
           (end.tv_sec - start.tv_sec) * 1000000000 + (
               end.tv_nsec - start.tv_nsec));

    atomic_store(&waker->started, 0);
    atomic_store(&waker->finished, 0);
    waker->futex_waker.futex = 0;

    for (int i = 0; i < SLEEPING_THREADS; i++) {
        pthread_create(&threads[i],
                       NULL,
                       futex_waker_thread,
                       waker);
    }

    while (atomic_load(&waker->started) != SLEEPING_THREADS)
        _mm_pause();

    usleep(600000);

    clock_gettime(CLOCK_MONOTONIC, &start);

    futex_wake(&waker->futex_waker.futex);

    while (atomic_load(&waker->finished) != SLEEPING_THREADS)
        _mm_pause();

    clock_gettime(CLOCK_MONOTONIC, &end);

    for (int i = 0; i < SLEEPING_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("FutexWaker: %ld\n",
           (end.tv_sec - start.tv_sec) * 1000000000 + (
               end.tv_nsec - start.tv_nsec));

    atomic_store(&waker->started, 0);
    atomic_store(&waker->finished, 0);
    waker->spin_waker.wake = false;

    for (int i = 0; i < SLEEPING_THREADS; i++) {
        pthread_create(&threads[i],
                       NULL,
                       spin_waker_thread,
                       waker);
    }

    while (atomic_load(&waker->started) != SLEEPING_THREADS)
        _mm_pause();

    usleep(600000);

    clock_gettime(CLOCK_MONOTONIC, &start);

    waker->spin_waker.wake = true;

    while (atomic_load(&waker->finished) != SLEEPING_THREADS)
        _mm_pause();

    clock_gettime(CLOCK_MONOTONIC, &end);

    for (int i = 0; i < SLEEPING_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("SpinWaker: %ld\n",
           (end.tv_sec - start.tv_sec) * 1000000000 + (
               end.tv_nsec - start.tv_nsec));

    free(waker);
}