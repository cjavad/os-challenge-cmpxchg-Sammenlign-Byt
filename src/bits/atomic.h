#pragma once

#ifdef __clang__
#define Atomic(T) _Atomic(T)
#else
#define Atomic(T) T
#endif