#pragma once

#include <stdlib.h>
#include <string.h>

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

#define ____freelist_maybe_grow(freelist, iii)                                                                         \
    {                                                                                                                  \
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
    }

#define freelist_insert(freelist, elem)                                                                                \
    ({                                                                                                                 \
        ____freelist_maybe_grow(freelist, __COUNTER__);                                                                \
        (freelist)->data[(freelist)->indicices[--(freelist)->free]] = (elem);                                          \
        (freelist)->indicices[(freelist)->free];                                                                       \
    })

#define freelist_insert_unsafe(freelist, ptr, size)                                                                    \
    ({                                                                                                                 \
        ____freelist_maybe_grow(freelist, __COUNTER__);                                                                \
        _Static_assert(sizeof(*(freelist)->data) == size, "Size mismatch");                                            \
        memcpy(&(freelist)->data[(freelist)->indicices[--(freelist)->free]], ptr, size);                               \
        (freelist)->indicices[(freelist)->free];                                                                       \
    })

#define freelist_remove(freelist, idx)                                                                                 \
    { (freelist)->indicices[(freelist)->free++] = (idx); }

#define freelist_destroy(freelist)                                                                                     \
    {                                                                                                                  \
        free((freelist)->data);                                                                                        \
        free((freelist)->indicices);                                                                                   \
    }
