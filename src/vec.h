#pragma once

#include <stdatomic.h>
#include <stdlib.h>

#define Vec(T)                                                                                                         \
    struct {                                                                                                           \
        T* data;                                                                                                       \
        uint32_t len;                                                                                                  \
        uint32_t cap;                                                                                                  \
    }

#define vec_init(vec, default_cap)                                                                                     \
    {                                                                                                                  \
        (vec)->len = 0;                                                                                                \
        (vec)->cap = (default_cap);                                                                                    \
        (vec)->data = calloc(((vec)->cap), sizeof(*(vec)->data));                                                      \
    }

#define vec_destroy(vec)                                                                                               \
    { free((vec)->data); }

#define vec_get(vec, idx)      ((vec)->data[(idx)])
#define vec_set(vec, idx, val) ((vec)->data[(idx)] = (val))

#define vec_push(vec, val)                                                                                             \
    {                                                                                                                  \
        if ((vec)->len == (vec)->cap) {                                                                                \
            (vec)->cap *= 2;                                                                                           \
            (vec)->data = reallocarray((vec)->data, (vec)->cap, sizeof(*(vec)->data));                                 \
        }                                                                                                              \
        (vec)->data[(vec)->len++] = (val);                                                                             \
    }

#define vec_pop(vec) ((vec)->data[--(vec)->len])

#define vec_foreach(vec, idx, elem) for (uint32_t idx = 0; idx < (vec)->len && ((elem) = &(vec)->data[idx]); idx++)

#define FreeList(T)                                                                                                    \
    struct {                                                                                                           \
        T* data;                                                                                                       \
        uint32_t* indicices;                                                                                           \
        uint32_t cap;                                                                                                  \
        uint32_t free;                                                                                                 \
    }

#define freelist_init(freelist, default_cap)                                                                           \
    {                                                                                                                  \
        (freelist)->cap = (default_cap);                                                                               \
        (freelist)->data = calloc(((freelist)->cap), sizeof(*(freelist)->data));                                       \
        (freelist)->indicices = calloc(((freelist)->cap), sizeof(*(freelist)->indicices));                             \
        (freelist)->free = (freelist)->cap;                                                                            \
        for (uint32_t i = 0; i < (freelist)->cap; i++) {                                                               \
            (freelist)->indicices[i] = i;                                                                              \
        }                                                                                                              \
    }

#define ____CONCAT_IMPL(a, b) a##b
#define ____CONCAT(a, b)      ____CONCAT_IMPL(a, b)
#define ____LAST_CAP(iii)     ____CONCAT(____lastcap____, iii)

#define ____freelist_add(freelist, elem, iii)                                                                          \
    ({                                                                                                                 \
        if ((freelist)->free == 0) {                                                                                   \
            uint32_t ____LAST_CAP(iii) = (freelist)->cap;                                                              \
            (freelist)->cap = (uint32_t)((float)(freelist)->cap * ((freelist)->cap <= (1 << 14) ? 2 : 1.25));          \
            (freelist)->data = reallocarray((freelist)->data, (freelist)->cap, sizeof(*(freelist)->data));             \
            (freelist)->indicices =                                                                                    \
                reallocarray((freelist)->indicices, (freelist)->cap, sizeof(*(freelist)->indicices));                  \
            for (uint32_t i = ____LAST_CAP(iii); i < (freelist)->cap; i++) {                                           \
                (freelist)->indicices[i - ____LAST_CAP(iii)] = i;                                                      \
            }                                                                                                          \
            (freelist)->free = (freelist)->cap - ____LAST_CAP(iii);                                                    \
        }                                                                                                              \
        (freelist)->data[(freelist)->indicices[--(freelist)->free]] = (elem);                                          \
        (freelist)->indicices[(freelist)->free];                                                                       \
    })

#define freelist_add(freelist, elem) ____freelist_add(freelist, elem, __COUNTER__)

#define freelist_remove(freelist, idx)                                                                                 \
    { (freelist)->indicices[(freelist)->free++] = (idx); }

#define freelist_destroy(freelist)                                                                                     \
    {                                                                                                                  \
        free((freelist)->data);                                                                                        \
        free((freelist)->indicices);                                                                                   \
    }

#define RingBuffer(T)                                                                                                  \
    struct {                                                                                                           \
        T* data;                                                                                                       \
        uint32_t size;                                                                                                 \
        uint32_t head;                                                                                                 \
        uint32_t tail;                                                                                                 \
    }

#define ringbuffer_head(rb)   (atomic_load(&(rb)->head))
#define ringbuffer_tail(rb)   (atomic_load(&(rb)->tail))
#define ringbuffer_length(rb) ((ringbuffer_head(rb) - ringbuffer_tail(rb)) % (rb)->size)

#define ringbuffer_init(rb, cap)                                                                                       \
    {                                                                                                                  \
        (rb)->size = (cap);                                                                                            \
        (rb)->data = calloc(cap, sizeof(*(rb)->data));                                                                 \
        atomic_store(&(rb)->head, 0);                                                                                  \
        atomic_store(&(rb)->tail, 0);                                                                                  \
    }

#define ringbuffer_destroy(rb)                                                                                         \
    { free((rb)->data); }

#define ringbuffer_push(rb, val)                                                                                       \
    {                                                                                                                  \
        uint32_t head = ringbuffer_head(rb);                                                                           \
        uint32_t tail = ringbuffer_tail(rb);                                                                           \
        if ((head + 1) % (rb)->size != tail) {                                                                         \
            (rb)->data[head] = (val);                                                                                  \
            atomic_store(&(rb)->head, (head + 1) % (rb)->size);                                                        \
        }                                                                                                              \
    }

#define ringbuffer_pop(rb, val)                                                                                        \
    {                                                                                                                  \
        uint32_t head = ringbuffer_head(rb);                                                                           \
        uint32_t tail = ringbuffer_tail(rb);                                                                           \
        if (head != tail) {                                                                                            \
            *(val) = (rb)->data[tail];                                                                                 \
            atomic_store(&(rb)->tail, (tail + 1) % (rb)->size);                                                        \
        }                                                                                                              \
    }

#define ringbuffer_empty(rb) (ringbuffer_head(rb) == ringbuffer_tail(rb))
#define ringbuffer_full(rb)  ((ringbuffer_head(rb) + 1) % (rb)->size == ringbuffer_tail(rb))