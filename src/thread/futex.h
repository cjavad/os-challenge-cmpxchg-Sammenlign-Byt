#pragma once

#include <errno.h>
#include <linux/futex.h>
#include <stdint.h>
#include <sys/time.h>
#include <syscall.h>
#include <unistd.h>
#include <stdbool.h>

int futex(uint32_t *uaddr, int futex_op, int val,
          const struct timespec *timeout, uint32_t *uaddr2, int val3);

void futex_wait(uint32_t *futex_ptr);
void futex_post(uint32_t *futex_ptr);

/* FUTEX2_ to FLAGS_ */
/*
 * Futex flags used to encode options to functions and preserve them across
 * restarts.
 */
#define FLAGS_SIZE_8		0x0000
#define FLAGS_SIZE_16		0x0001
#define FLAGS_SIZE_32		0x0002
#define FLAGS_SIZE_64		0x0003

#define FLAGS_SIZE_MASK		0x0003

#ifdef CONFIG_MMU
# define FLAGS_SHARED		0x0010
#else
/*
 * NOMMU does not have per process address space. Let the compiler optimize
 * code away.
 */
# define FLAGS_SHARED		0x0000
#endif
#define FLAGS_CLOCKRT		0x0020
#define FLAGS_HAS_TIMEOUT	0x0040
#define FLAGS_NUMA		0x0080
#define FLAGS_STRICT		0x0100
#define FUTEX2_VALID_MASK (FUTEX2_SIZE_MASK | FUTEX2_PRIVATE)

static inline unsigned int futex2_to_flags(unsigned int flags2)
{
  unsigned int flags = flags2 & FUTEX2_SIZE_MASK;

  if (!(flags2 & FUTEX2_PRIVATE))
    flags |= FLAGS_SHARED;

  if (flags2 & FUTEX2_NUMA)
    flags |= FLAGS_NUMA;

  return flags;
}

static inline unsigned int futex_size(unsigned int flags)
{
  return 1 << (flags & FLAGS_SIZE_MASK);
}

static inline bool futex_flags_valid(unsigned int flags)
{
  /* Only 64bit futexes for 64bit code */
  if (true) {
    if ((flags & FLAGS_SIZE_MASK) == FLAGS_SIZE_64)
      return false;
  }

  /* Only 32bit futexes are implemented -- for now */
  if ((flags & FLAGS_SIZE_MASK) != FLAGS_SIZE_32)
    return false;

  return true;
}

static inline bool futex_validate_input(unsigned int flags, uint64_t val)
{
  int bits = 8 * futex_size(flags);

  if (bits < 64 && (val >> bits))
    return false;

  return true;
}