#pragma once

#include <stdlib.h>

#define Vec(T)                                                                                                         \
    struct {                                                                                                           \
        T* data;                                                                                                       \
        uint32_t len;                                                                                                  \
        uint32_t cap;                                                                                                  \
    }

#define VecArr(T, N)                                                                                                   \
    struct {                                                                                                           \
        T data[N];                                                                                                     \
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

#define vec_get(vec, idx) ((vec)->data[(idx)])
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

#define FreeList(T) \
    struct {                 \
        T* data;               \
        uint32_t* indicices; \
        uint32_t  cap; \
        uint32_t  free;       \
    }

#define freelist_init(freelist, default_cap) \
    { \
        (freelist)->cap = (default_cap); \
        (freelist)->data = calloc(((freelist)->cap), sizeof(*(freelist)->data)); \
        (freelist)->indicices = calloc(((freelist)->cap), sizeof(*(freelist)->indicices)); \
        (freelist)->free = (freelist)->cap; \
        for (uint32_t i = 0; i < (freelist)->cap; i++) { \
            (freelist)->indicices[i] = i; \
        } \
    }

#define freelist_add(freelist, elem) \
    ({ \
        if ((freelist)->free == 0) { \
            (freelist)->cap *= 2; \
            (freelist)->data = reallocarray((freelist)->data, (freelist)->cap, sizeof(*(freelist)->data)); \
            (freelist)->indicices = reallocarray((freelist)->indicices, (freelist)->cap, sizeof(*(freelist)->indicices)); \
            for (uint32_t i = (freelist)->cap / 2; i < (freelist)->cap; i++) { \
                (freelist)->indicices[i - (freelist)->cap / 2] = i; \
            } \
            (freelist)->free += (freelist)->cap / 2; \
        } \
        (freelist)->data[(freelist)->indicices[--(freelist)->free]] = (elem); \
    (freelist)->indicices[(freelist)->free]; })

#define freelist_remove(freelist, idx) \
    { \
        (freelist)->indicices[(freelist)->free++] = (idx); \
    }

#define freelist_destroy(freelist) \
    { \
        free((freelist)->data); \
        free((freelist)->indicices); \
    }
