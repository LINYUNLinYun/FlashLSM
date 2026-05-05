# FlashLSM

FlashLSM is a course-project key-value store implemented with a minimal LSM-Tree design. It focuses on correctness, readability, and explainability rather than industrial completeness.

## Features

- `put(key, value)`, `get(key)`, and `remove(key)`
- Ordered in-memory MemTable
- Write-Ahead Log for recovery
- SSTable flush with an in-memory sparse index
- Tombstone-based deletion
- Recovery after process restart
- Newest-record-wins read path
- Simple compaction for old SSTables
- Correctness tests and a small benchmark executable

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Run Tests

```bash
ctest --test-dir build --output-on-failure
```

The tests cover MemTable behavior, SSTable round trips, WAL recovery, flush/read correctness, tombstones, and compaction.

## Run Demo

```bash
./build/FlashLSM_demo
```

The demo performs a small sequence of writes, reads, deletes, and flushes.

## Run Benchmark

```bash
./build/FlashLSM_benchmark
```

The benchmark reports:

- sequential write throughput
- random hit-read latency before/after compaction
- missing-key read latency before/after compaction
- SSTable count and disk usage before/after compaction

The numbers are intended for course-report discussion, not as a production-grade performance claim.

## Documentation

- [Design](docs/design.md)
- [Storage format](docs/storage-format.md)
- [Roadmap](docs/roadmap.md)
- [Report notes](docs/report-notes.md)

## Current Scope

FlashLSM implements the core LSM data path:

```text
put/remove -> WAL -> MemTable -> flush -> SSTable -> get/recovery/compaction
```

Optional future work includes Bloom Filters, an LRU cache, block-based SSTables, and background compaction.
