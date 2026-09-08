# Redis-Compatible Key-Value Server

A Redis-inspired in-memory key-value store built from raw TCP sockets, using a non-blocking, edge-triggered epoll event loop and a multi-threaded worker pool.

## Architecture

```
Client ── TCP ──▶ Socket (listener, non-blocking)
                       │
                       ▼
              epoll event loop (per worker thread)
                       │
                       ▼
              Connection (buffered read/write, state)
                       │
                       ▼
              Protocol parser → Command dispatcher
                       │
                       ▼
              Store (shared, mutex-protected)
```

Each worker thread runs its own epoll instance and owns a subset of client connections. All workers share one `Store` instance, synchronized via a mutex to prevent lost updates on concurrent writes to the same key.

## Correctness testing

Verified with a Python-based integration test suite covering:
- Basic command correctness, including missing-key behavior
- TTL expiry (key becomes inaccessible after its expiry window)
- Partial/fragmented sends: commands sent byte-by-byte across multiple TCP packets are correctly reassembled by the buffering layer
- Concurrent write correctness: 8 client threads incrementing the same key concurrently, verified the final value matches exactly, confirming no lost updates or race conditions

## Benchmarks

Measured with a concurrent Python load generator (multiple client threads, each issuing a stream of `SET`/`GET` requests over its own connection, loopback).

| Concurrency | Ops/thread | Total ops | ops/sec | p50 | p99 |
|---|---|---|---|---|---|
| 1 client | 500 | 1,000 | ~19,600 | ~84µs | ~245µs |
| 16 clients | 50,000 | 1,600,000 | ~154,000–163,000 | ~150–160µs | ~460–500µs |
| 50 clients | 500 | 50,000 | ~146,000 | ~469µs | ~1,687µs |

**Findings:**
- Throughput scaled roughly 6–8x going from 1 to 16 concurrent clients.
- Beyond ~16 clients, throughput plateaued and both p50 and p99 latency increased substantially — most likely caused by contention on the single mutex guarding the shared Store. This is the current known bottleneck.

## Build & run

```bash
# compile
make 

# compile + run
make run

# remove .o and executable files
make clean
```

## Testing

```bash
python3 test.py
```

Runs correctness tests followed by load/benchmark tests.

## Known limitations / future work

- No persistence — data is lost on restart (append-only-file logging is a planned addition)
- Shard the store to reduce lock contention at higher concurrency.
- Command set is intentionally limited (`GET`/`SET`/`DEL`/`EXISTS`/`INCR`/`DECR`/`EXPIRE`/`TTL`) rather than covering full Redis compatibility