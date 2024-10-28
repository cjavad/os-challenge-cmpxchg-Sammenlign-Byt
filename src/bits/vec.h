#pragma once

#include <stdint.h>
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

#define __vec_maybe_grow(vec, size)                                                                                    \
    {                                                                                                                  \
        if ((vec)->len == (vec)->cap) {                                                                                \
            (vec)->cap *= 2;                                                                                           \
            (vec)->data = reallocarray((void*)(vec)->data, (vec)->cap, size);                                          \
        }                                                                                                              \
    }

#define vec_push(vec, val)                                                                                             \
    {                                                                                                                  \
        __vec_maybe_grow(vec, sizeof(*(vec)->data));                                                                   \
        vec_set(vec, (vec)->len++, val);                                                                               \
    }

#define vec_pop(vec) vec_get(vec, --(vec)->len)

#define __vec_copy(dest, src, size)                                                                                    \
    {                                                                                                                  \
        (dest)->len = (src)->len;                                                                                      \
        (dest)->cap = (src)->cap;                                                                                      \
        (dest)->data = reallocarray((dest)->data, (dest)->cap, size);                                                  \
        memcpy((void*)(dest)->data, (void*)(src)->data, ((dest)->len) * size);                                          \
    }

#define vec_copy(dest, src) __vec_copy(dest, src, sizeof(*(src)->data))

#define vec_get_unsafe(vec, idx, size) ({ (((void*)(vec)->data) + ((idx) * (size))); })

#define vec_set_unsafe(vec, idx, val, size) memcpy(((void*)(vec)->data) + ((idx) * (size)), val, size)
#define vec_push_unsafe(vec, val, size)                                                                                \
    {                                                                                                                  \
        __vec_maybe_grow(vec, size);                                                                                   \
        vec_set_unsafe(vec, (vec)->len++, val, size);                                                                  \
    }

#define vec_pop_unsafe(vec, size)        vec_get_unsafe(vec, --(vec)->len, size)
#define vec_copy_unsafe(dest, src, size) __vec_copy(dest, src, size)