# LSM-Tree KV Store Design

## 1. Project Objective

This project implements a minimal LSM-Tree based key-value storage engine. The system is designed for write-heavy workloads by buffering writes in memory and periodically flushing ordered immutable files to disk.

The core target is not to reproduce RocksDB completely, but to build a small, understandable storage engine that demonstrates the main ideas of LSM-Tree systems.

## 2. Public API

```cpp
class KVStore {
public:
    void put(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key);
    void remove(const std::string& key);
};
```

`remove()` is implemented by writing a tombstone rather than deleting data immediately.

## 3. Core Components

### 3.1 MemTable

The MemTable is an ordered in-memory map. It receives all recent writes.

Suggested implementation:

```cpp
std::map<std::string, Entry>
```

Each entry stores:

- key
- value
- sequence number
- record type: value or tombstone

### 3.2 WAL

The Write-Ahead Log is append-only. Every write must be written to the WAL before being inserted into the MemTable.

Purpose:

- support crash recovery
- preserve write ordering
- avoid losing recent writes not yet flushed to SSTable

### 3.3 SSTable

An SSTable is an immutable sorted file generated from a MemTable flush.

Basic properties:

- sorted by key
- immutable after creation
- has an index for lookup
- newer SSTables have higher sequence/file number

### 3.4 Flush

When MemTable reaches a threshold, it is flushed to a new SSTable.

Simplified flush process:

1. Freeze current MemTable.
2. Write sorted entries to a new SSTable file.
3. Reset MemTable.
4. Clear or rotate WAL after successful flush.

### 3.5 Read Path

The read path should search in the following order:

1. MemTable
2. SSTables from newest to oldest

If a tombstone is found, return `not found`.

### 3.6 Compaction

Compaction is optional for MVP but recommended as an extension.

Purpose:

- merge overlapping SSTables
- remove stale versions
- remove tombstones when safe
- reduce read amplification

## 4. Data Freshness Rule

When the same key appears in multiple places, the newest version wins.

Priority order:

1. MemTable
2. Newer SSTable
3. Older SSTable

This can be implemented using sequence numbers or file creation order.

## 5. MVP Invariants

The implementation should preserve these invariants:

- WAL is appended before MemTable update.
- SSTable files are immutable.
- MemTable entries are sorted before flush.
- Newer records override older records.
- Tombstone means deleted.
- Recovery rebuilds MemTable from WAL.

## 6. Optional Optimizations

### LRU Cache

Used in the read path to cache hot key-value pairs.

### Bloom Filter

Used before searching an SSTable to avoid unnecessary disk reads.

### Block-based SSTable

Splits SSTable into blocks and caches blocks instead of individual keys.

### Benchmark

Measure:

- write throughput
- read latency
- read amplification
- effect of compaction
- effect of cache
