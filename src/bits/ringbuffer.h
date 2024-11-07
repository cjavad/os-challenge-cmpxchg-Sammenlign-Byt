#pragma once

#include <stdatomic.h>
#include <stdlib.h>

#define RingBuffer(T)                                                          \
    struct {                                                                   \
        T* data;                                                               \
        uint32_t size;                                                         \
        uint32_t head;                                                         \
        uint32_t tail;                                                         \
    }

#define ringbuffer_head(rb) (atomic_load(&(rb)->head))
#define ringbuffer_tail(rb) (atomic_load(&(rb)->tail))
#define ringbuffer_length(rb)                                                  \
    ((ringbuffer_head(rb) - ringbuffer_tail(rb)) % (rb)->size)

#define ringbuffer_init(rb, cap)                                               \
    {                                                                          \
        (rb)->size = (cap);                                                    \
        (rb)->data = calloc(cap, sizeof(*(rb)->data));                         \
        atomic_store(&(rb)->head, 0);                                          \
        atomic_store(&(rb)->tail, 0);                                          \
    }

#define ringbuffer_destroy(rb)                                                 \
    {                                                                          \
        free((rb)->data);                                                      \
    }

#define ringbuffer_push(rb, val)                                               \
    {                                                                          \
        uint32_t head = ringbuffer_head(rb);                                   \
        uint32_t tail = ringbuffer_tail(rb);                                   \
        if ((head + 1) % (rb)->size != tail) {                                 \
            (rb)->data[head] = (val);                                          \
            atomic_store(&(rb)->head, (head + 1) % (rb)->size);                \
        }                                                                      \
    }

#define ringbuffer_pop(rb, val)                                                \
    {                                                                          \
        uint32_t head = ringbuffer_head(rb);                                   \
        uint32_t tail = ringbuffer_tail(rb);                                   \
        if (head != tail) {                                                    \
            *(val) = (rb)->data[tail];                                         \
            atomic_store(&(rb)->tail, (tail + 1) % (rb)->size);                \
        }                                                                      \
    }

#define ringbuffer_empty(rb) (ringbuffer_head(rb) == ringbuffer_tail(rb))
#define ringbuffer_full(rb)                                                    \
    ((ringbuffer_head(rb) + 1) % (rb)->size == ringbuffer_tail(rb))
