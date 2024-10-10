import math
from dataclasses import dataclass
import time
from typing import List

import cffi
import ctypes


@dataclass
class Request:
    val: int
    start: int
    end: int
    prio: int


class Params:
    seed = 432984
    total = 1
    start = 0
    difficulty = 30_000_000
    lamd = 1.5
    repetition_probability = 20
    delay = 0

    @property
    def priority(self):
        return self.lamd != 0

    @property
    def start_randomisation(self):
        return self.start == 0


ffi = cffi.FFI()

ffi.cdef('''
    int rand(void);
    void srand(unsigned int seed);
''')

C = ffi.dlopen(None)

g_idx = 0


def reset_rand(seed: int):
    global g_idx
    g_idx = 0
    if seed == 0:
        seed = time.time()
    C.srand(seed)


def rand():
    global g_idx
    val = C.rand()
    g_idx += 1
    return val


def left_shift_with_overflow(val, shift, bit_size=64):
    max_value = (1 << bit_size) - 1  # 0xFFFFFFFFFFFFFFFF for 64-bit
    return (val << shift) & max_value  # Apply the shift and


def ran_expo(params: Params):
    r = rand()
    l = math.log(1.0 - (r / 0x80000000))
    l = ctypes.c_double(l).value
    l = l / params.lamd
    return (int(l) & 0x7FFFFFFF) + 1


def signed_to_unsigned(val):
    bs = int.to_bytes(val, 8, byteorder='little', signed=True)
    return int.from_bytes(bs, byteorder='little', signed=False)


def generate_requests(params: Params):
    requests: List[Request | None] = [None for _ in range(params.total)]

    for i in range(params.total):
        if params.start_randomisation:
            a = rand()
            start = left_shift_with_overflow(a, 32) | rand()
            start = start % signed_to_unsigned(-params.difficulty - 2)

        r = rand()
        r = r % 100

        if i == 0 or params.repetition_probability < r:
            x = rand()
            v = params.start + (x % params.difficulty)
            requests[i] = Request(v, params.start, params.difficulty + params.start, 1)
        else:
            x = rand()
            requests[i] = requests[x % i]

        if params.priority:
            p = ran_expo(params)

            if 0x10 < p:
                p = 0x10

            requests[i].prio = p

    return requests


def reverse_requests(params: Params, reqs: List[Request]):
    # Attempt a lot of different seeds.
    for i in range(1, 2_500_000):
        reset_rand(i)
        nreqs = generate_requests(params)
        valid = True
        # Compare the generated requests with the original ones.
        for j in range(len(reqs)):
            if reqs[j].val != nreqs[j].val:
                valid = False
                break

        if valid:
            return i

    return None


ps = Params()
reset_rand(ps.seed)
rs = generate_requests(ps)
print(reverse_requests(ps, rs))
