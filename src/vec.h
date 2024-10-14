#pragma once

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

