# RedKV

An event-driven key-value store built from scratch in C++

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)]()
[![License](https://img.shields.io/badge/license-MIT-blue)]()
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue)]()

---

## Quick Start

```bash
# Build
make

# Run server
./bin/redis-server

# Test with redis-cli
redis-cli -p 6379 PING
redis-cli -p 6379 SET mykey "hello"
redis-cli -p 6379 GET mykey
```

---

## Architecture Overview

```
┌─────────────┐
│   Clients   │
└──────┬──────┘
       │ RESP Protocol
┌──────▼──────────────┐
│  Event Loop (epoll) │
│  - Non-blocking I/O │
│  - Edge-triggered   │
│  - Single-threaded  │
└──────┬──────────────┘
       │
┌──────▼──────────────┐
│ Command Processor   │
└──────┬──────────────┘
       │
┌──────▼──────────────┐
│ Hash Table Storage  │
│ - O(1) access       │
│ - Lazy expiration   │
└─────────────────────┘
```

Key Features:

* Single-threaded event loop (epoll)
* Non-blocking I/O everywhere
* Command pipelining support
* 10,000+ concurrent connections
* Zero locks in critical path

---

## Supported Commands

| Command              | Description                    | Example                     |
| -------------------- | ------------------------------ | --------------------------- |
| PING [msg]           | Test connectivity              | PING → PONG                 |
| ECHO msg             | Echo message                   | ECHO "hello" → "hello"      |
| GET key              | Get value                      | GET mykey → "myvalue"       |
| SET key val [EX sec] | Set value with optional expiry | SET key "val" EX 60         |
| DEL key [key...]     | Delete keys                    | DEL key1 key2 → (integer) 2 |
| EXISTS key [key...]  | Check existence                | EXISTS key1 → (integer) 1   |
| TTL key              | Get time-to-live               | TTL key → (integer) 58      |
| DBSIZE               | Count keys                     | DBSIZE → (integer) 42       |

---

## Performance Benchmarks

### Fixed Sample Size Latency

Command:

```bash
redis-cli --latency-history -i 15
```

Our Server:

```
min: 0, max: 4, avg: 0.12 (1467 samples) -- 15.01 seconds range
min: 0, max: 1, avg: 0.13 (1465 samples) -- 15.01 seconds range
min: 0, max: 2, avg: 0.12 (1466 samples) -- 15.01 seconds range
min: 0, max: 2, avg: 0.13 (1466 samples) -- 15.00 seconds range
min: 0, max: 2, avg: 0.13 (1466 samples) -- 15.01 seconds range
```

Redis Official:

```
min: 0, max: 1, avg: 0.13 (1464 samples) -- 15.01 seconds range
min: 0, max: 1, avg: 0.13 (1464 samples) -- 15.01 seconds range
min: 0, max: 1, avg: 0.14 (1463 samples) -- 15.01 seconds range
min: 0, max: 2, avg: 0.13 (1463 samples) -- 15.00 seconds range
min: 0, max: 2, avg: 0.14 (1464 samples) -- 15.01 seconds range
```

Summary:

* Average latency comparable to Redis
* Occasional higher tail latency due to simpler networking stack
* Stable under sustained load

---

### Throughput Benchmark

Command:

```bash
redis-benchmark -t get,set -n 1000000 -c 50 -q
```

Our Server:

```
SET: 104865.77 requests per second, p50=0.231 msec
GET: 105663.57 requests per second, p50=0.239 msec
```

Redis Official:

```
SET: 105797.72 requests per second, p50=0.239 msec
GET: 105842.51 requests per second, p50=0.239 msec
```

Observations:

* Comparable single-command throughput
* Slightly worse tail latency
* Command execution path is not the bottleneck

---

### Pipelined Performance

Command:

```bash
redis-benchmark -t set -n 1000000 -P 100 -c 50 -q
```

Our Server:

```
SET: 192789.67 requests per second, p50=24.479 msec
```

Redis:

```
SET: 1459854.00 requests per second, p50=3.167 msec
```

Interpretation:

* Redis massively outperforms under pipelining
* Indicates write aggregation, buffer management, and syscall batching are immature
* Known limitation of Phase 1 design

---

### Memory Usage

10,000 concurrent idle connections:

* Our Server: ~180 MB RSS (~18 KB per connection)
* Redis: ~120 MB RSS (~12 KB per connection)

1M keys, 100-byte values:

* Our Server: ~185 MB
* Redis: ~165 MB

Memory overhead is higher but within acceptable bounds for an early implementation.

---

## System Design

### Event Loop (epoll)

```cpp
while (running) {
    events = epoll_wait(epoll_fd, events, MAX_EVENTS, 100ms);

    for each event:
        if (event.fd == listen_socket):
            accept_new_connections()
        else if (event & EPOLLIN):
            connection.try_read()
            process_commands(connection)
        else if (event & EPOLLOUT):
            connection.try_write()
}
```

Edge-triggered epoll is used for throughput and reduced wakeups. This increases complexity but forces correct non-blocking semantics.

Single-threaded execution avoids locks and matches Redis’ core model.

---

### RESP Protocol Parsing

Incremental state machine parsing, handling partial reads and pipelined commands without buffering entire requests. Parsing progresses as data arrives, with bounded memory usage and no blocking behavior.

---

### Hash Table Storage

* Separate chaining with linked lists
* Average O(1) access
* Lazy expiration on access
* No resizing or rehashing yet

This is intentionally simple and correct first. Performance improvements come later.

---

## Project Structure

```
redis-server/
├── src/
│   ├── server/
│   │   ├── EventLoop.{h,cpp}
│   │   ├── Connection.{h,cpp}
│   │   └── Server.{h,cpp}
│   ├── protocol/
│   │   └── RESPParser.{h,cpp}
│   ├── store/
│   │   ├── HashTable.{h,cpp}
│   │   └── RedKV.{h,cpp}
│   ├── commands/
│   │   └── Command.{h,cpp}
│   └── main.cpp
├── Makefile
└── README.md
```

---

## Current Limitations

* No persistence (AOF / RDB)
* No rehashing or resizing
* Single-threaded I/O
* Lazy expiration only
* String-only values
* No memory limits or eviction
* No authentication
* No replication

These are architectural and early-stage, not accidental.
---

## Future Enhancements

* Full Redis data type system
* Active expiration cycle
* Memory eviction and maxmemory support
* Durable persistence (AOF + RDB)
* Replication and Pub/Sub
* Operational commands and introspection

---

## Development

### Building

```bash
make clean
make CXXFLAGS="-O3 -DNDEBUG"
```

### Debugging

```bash
gdb ./bin/redis-server
valgrind --leak-check=full ./bin/redis-server
strace -e epoll_wait,recv,send ./bin/redis-server
```

---

## Contributing

* C++17
* RAII everywhere
* clang-format
* Clear commits, no noise

---

## Acknowledgments

Inspired by Redis. Built from scratch for learning and systems-level understanding. Uses Linux epoll for event-driven I/O.