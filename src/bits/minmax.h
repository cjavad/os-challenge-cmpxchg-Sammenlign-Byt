#pragma once
#include "concat.h"

#define __minmax_impl(op, a, b, c)                                             \
    ({                                                                         \
        __auto_type __CONCAT(_a, c) = (a);                                     \
        __auto_type __CONCAT(_b, c) = (b);                                     \
        __CONCAT(_a, c)                                                        \
            op __CONCAT(_b, c) ? __CONCAT(_a, c) : __CONCAT(_b, c);            \
    })

#define min(a, b) __minmax_impl(<, a, b, __COUNTER__)
#define max(a, b) __minmax_impl(>, a, b, __COUNTER__)
