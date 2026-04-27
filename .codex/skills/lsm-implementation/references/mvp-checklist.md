# Development Roadmap

## Phase 1: MVP Correctness

Goal: produce a complete minimal LSM KV Store.

Tasks:

1. Create `KVStore` public API.
2. Implement `MemTable` with `std::map`.
3. Implement WAL append.
4. Implement WAL recovery.
5. Implement flush from MemTable to SSTable.
6. Implement SSTable lookup.
7. Implement tombstone deletion.
8. Add tests.

Expected result:

- `put/get/delete` work.
- Data survives process restart if WAL exists.
- Data survives flush to SSTable.

## Phase 2: Simple Compaction

Goal: reduce duplicate entries across SSTables.

Tasks:

1. Select several old SSTables.
2. Merge records by key.
3. Keep newest record.
4. Write merged output as one new SSTable.
5. Remove old SSTables only after successful output creation.

Expected result:

- Fewer SSTables.
- Reads become faster.
- Old versions are cleaned.

## Phase 3: Benchmark

Goal: quantify system behavior.

Tasks:

1. Sequential write benchmark.
2. Random read benchmark.
3. Read-after-write benchmark.
4. Compare before and after compaction.
5. Optionally compare with and without cache.

Metrics:

- write throughput
- average read latency
- number of SSTables checked per read
- disk space usage

## Phase 4: Advanced Optimizations

Optional tasks:

1. Bloom Filter per SSTable.
2. LRU key-value cache.
3. Block-based SSTable.
4. Background compaction thread.
5. Range scan.

## Suggested Timeline

| Day | Goal |
|---|---|
| Day 1 | API, MemTable, basic tests |
| Day 2 | WAL append and recovery |
| Day 3 | SSTable flush and lookup |
| Day 4 | delete/tombstone and integration tests |
| Day 5 | simple compaction |
| Day 6 | benchmark and report notes |
| Day 7+ | Bloom Filter or LRU cache |
