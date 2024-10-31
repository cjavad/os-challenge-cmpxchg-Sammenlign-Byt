import math
import socket
import random
import struct
import hashlib
import time

difficulty = 30_000_000

def generate_priority(lambd):
    # 1 + (ln(U) / lambda^3 % 15
    return math.floor(1 + ((-math.log(1 - random.random()) / (lambd ** 3)) % 15))


def generate_protocol_parameters():
    s = random.randint(0, 2 ** 64 - difficulty)  # Pick a valid 64-bit unsigned int as start
    e = s + difficulty  # End is start + 30,000,000
    x = random.randint(s, e)
    x_bytes = x.to_bytes(8, byteorder='little')  # Convert x to little-endian bytes
    h = hashlib.sha256(x_bytes).digest()  # Calculate SHA-256 hash
    p = generate_priority(1.5)
    return x, h, s, e, p


def pack_protocol_request(h, s, e, p):
    # Pack the struct in big-endian format
    # The struct format: 32 bytes hash, 2x 64-bit unsigned integers (start, end), and 1 byte for priority
    return struct.pack('>32sQQB', h, s, e, p)


def request_server(host, port, data):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((host, port))
        start = time.monotonic_ns()
        s.send(data)
        answer = s.recv(8)
        end = time.monotonic_ns()
        return int.from_bytes(answer, byteorder='big'), math.floor((end - start) / 1000)


stats = []
requests_to_make = 100

for _ in range(requests_to_make):
    x, h, s, e, p = generate_protocol_parameters()
    data = pack_protocol_request(h, s, e, p)
    result, time_taken = request_server('localhost', 8080, data)
    stats.append((x == result, time_taken, p))
    print(f"[{int(x == result)} {x} {result} {p}] {time_taken}")

# Output % success and average time 1/total * sum(time * p)
print(f"Success rate: {sum(x[0] for x in stats)}%")
print(f"Average time: {sum(x[1] * x[2] for x in stats) / len(stats)} microseconds.")
