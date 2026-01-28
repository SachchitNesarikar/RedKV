# RedKV
#### A minimal Redis-compatible key–value server written in C++.

### What this is??
RedKV is an educational Redis clone that implements a subset of the Redis protocol (RESP) and core string commands. It is built from scratch using raw sockets, manual RESP parsing, and a thread-safe in-memory store.

This project focuses on systems fundamentals: networking, concurrency, protocol parsing, and data-store correctness. It is not production-ready and does not aim to replace Redis.

### What this is **NOT**
- Not production-grade
- No persistence (data is lost on restart)
- No eviction or memory limits
- No replication, clustering, or transactions
- No security hardening

### Current functionality

#### Supported commands:
- PING [message]
- ECHO message
- SET key value [EX seconds | PX milliseconds | EXAT unix_ts | PXAT unix_ms]
- GET key
- DEL key [key ...]
- TTL key
- KEYS pattern
- EXISTS key [key ...]
- SCAN cursor [MATCH pattern] [COUNT n]
- CONFIG (stub, returns fixed response)
- COMMAND (stub, for redis-cli compatibility)

#### Behavior:
- RESP-compatible with `redis-cli`
- Thread-safe concurrent access using `std::shared_mutex`
- One OS thread per client connection
- Lazy expiry on access (GET, DEL, TTL)
- SCAN provides weakly-consistent iteration
- Expired keys may appear briefly during SCAN
- Expired keys are cleaned opportunistically during access and scans
- Malformed client input never crashes the server


#### Unsupported commands:
All other Redis commands are unsupported and return:
ERR unknown command

### Build instructions

This project does not use CMake yet. Build manually.

#### From the project root:

> g++ -std=gnu++20 server.cpp cmd.cpp RedKV.cpp RESPParser.cpp utils.cpp -pthread -o redkv 

#### Run

> ./redkv 

#### Expected output:
Server listening on port: 2000 

#### Test using redis-cli:

> redis-cli -p 2000

> PING

> SET a 1

> TTL a

> GET a

> DEL a

> SCAN 0

### Concurrency model

- One OS thread per client connection
- Shared in-memory store protected by std::shared_mutex
- Reads use shared_lock, writes and deletions use unique_lock
- Expired keys are deleted lazily using scoped locking
- No stop-the-world operations

### Expiry model

- Expiry timestamps are stored per key
- Keys are checked for expiry on access
- Expired keys are lazily deleted
- EXISTS and TTL do not mutate state
- No background expiry thread yet

### Known limitations

- No persistence (RDB/AOF not implemented)
- No background expiry thread
- No incremental rehashing yet
- KEYS is blocking and O(n)
- No graceful shutdown
- No memory accounting or eviction
- No authentication or access control