#pragma once

#include "concat.h"
#include <stdlib.h>
#include <string.h>

#define FreeList(T)                                                            \
    struct {                                                                   \
        T* data;                                                               \
        uint32_t* indicices;                                                   \
        uint32_t cap;                                                          \
        uint32_t free;                                                         \
    }

#define ____freelist_init(freelist, default_cap, size)                         \
    {                                                                          \
        (freelist)->cap = (default_cap);                                       \
        (freelist)->data = calloc(((freelist)->cap), size);                    \
        (freelist)->indicices =                                                \
            calloc(((freelist)->cap), sizeof(*(freelist)->indicices));         \
        (freelist)->free = (freelist)->cap;                                    \
        for (uint32_t i = 0; i < (freelist)->cap; i++) {                       \
            (freelist)->indicices[i] = i;                                      \
        }                                                                      \
    }

#define freelist_init(freelist, default_cap)                                   \
    ____freelist_init(freelist, default_cap, sizeof(*(freelist)->data))
#define freelist_init_unsafe(freelist, default_cap, size)                      \
    ____freelist_init(freelist, default_cap, size)

#define ____LAST_CAP(iii) ____CONCAT(____lastcap____, iii)

#define ____freelist_maybe_grow(freelist, size, iii)                           \
    {                                                                          \
        if ((freelist)->free == 0) {                                           \
            uint32_t ____LAST_CAP(iii) = (freelist)->cap;                      \
            (freelist)->cap =                                                  \
                (uint32_t)((float)(freelist)->cap *                            \
                           ((freelist)->cap <= (1 << 14) ? 2 : 1.25));         \
            (freelist)->data =                                                 \
                reallocarray((freelist)->data, (freelist)->cap, size);         \
            (freelist)->indicices = reallocarray(                              \
                (freelist)->indicices,                                         \
                (freelist)->cap,                                               \
                sizeof(*(freelist)->indicices)                                 \
            );                                                                 \
            for (uint32_t i = ____LAST_CAP(iii); i < (freelist)->cap; i++) {   \
                (freelist)->indicices[i - ____LAST_CAP(iii)] = i;              \
            }                                                                  \
            (freelist)->free = (freelist)->cap - ____LAST_CAP(iii);            \
        }                                                                      \
    }

#define freelist_insert(freelist, elem)                                        \
    ({                                                                         \
        ____freelist_maybe_grow(                                               \
            freelist,                                                          \
            (sizeof(*(freelist)->data)),                                       \
            __COUNTER__                                                        \
        );                                                                     \
        (freelist)->data[(freelist)->indicices[--(freelist)->free]] = (elem);  \
        (freelist)->indicices[(freelist)->free];                               \
    })

#define freelist_insert_unsafe(freelist, ptr, size)                            \
    ({                                                                         \
        ____freelist_maybe_grow(freelist, (size), __COUNTER__);                \
        memcpy(                                                                \
            ((void*)(freelist)->data) +                                        \
                (((freelist)->indicices[--(freelist)->free]) * size),          \
            ptr,                                                               \
            size                                                               \
        );                                                                     \
        (freelist)->indicices[(freelist)->free];                               \
    })

#define freelist_get_unsafe(freelist, idx, size)                               \
    ({                                                                         \
        (typeof((freelist)->data))(((void*)(freelist)->data) +                 \
                                   ((idx) * (size)));                          \
    })

#define freelist_remove(freelist, idx)                                         \
    {                                                                          \
        (freelist)->indicices[(freelist)->free++] = (idx);                     \
    }

#define freelist_destroy(freelist)                                             \
    {                                                                          \
        free((freelist)->data);                                                \
        free((freelist)->indicices);                                           \
    }
