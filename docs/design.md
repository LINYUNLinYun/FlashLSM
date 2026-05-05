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
    void flush();
    void compact();
};
```

`remove()` is implemented by writing a tombstone rather than deleting data immediately.

## 3. Core Components

### Component Responsibilities

| Component | Responsibility |
|---|---|
| `KVStore` | Public API, sequence number allocation, read/write coordination, flush, recovery, compaction trigger |
| `MemTable` | Ordered in-memory records and approximate memory-size tracking |
| `WriteAheadLog` | Durable append-before-memory write path and replay during recovery |
| `SSTable` | Immutable sorted file creation, opening, index loading, point lookup, full record scan for compaction |
| `Compaction` | Select old SSTables, merge records, drop safe tombstones, replace old files |

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
- regular flushes create higher file numbers for newer SSTables
- after compaction, read freshness is determined by record sequence numbers, because a newly written compacted file may contain older logical data

### 3.4 Flush

When MemTable reaches a threshold, it is flushed to a new SSTable.

Simplified flush process:

1. Freeze current MemTable.
2. Write sorted entries to a new SSTable file.
3. Reset MemTable.
4. Clear or rotate WAL after successful flush.

The implementation writes the SSTable first, inserts it into the in-memory SSTable list, then resets the WAL and clears the MemTable. This ordering avoids losing records before they have a disk representation.

### 3.5 Read Path

The read path should search in the following order:

1. MemTable
2. SSTables from newest to oldest by record sequence number

If a tombstone is found, return `not found`.

Text flow:

```text
get(key)
  -> check MemTable
  -> check SSTables newest-to-oldest
  -> return value, not found, or tombstone-as-not-found
```

### 3.6 Compaction

Compaction is optional for MVP but recommended as an extension.

Purpose:

- merge overlapping SSTables
- remove stale versions
- remove tombstones when safe
- reduce read amplification

Current simple compaction flow:

```text
compact()
  -> select the oldest N SSTables
  -> read all records from selected files
  -> for each key, keep the newest record by sequence number
  -> drop a tombstone only when no older un-compacted SSTable can contain that key
  -> write one new SSTable
  -> replace selected SSTables in the in-memory list
  -> remove old SSTable files
```

Compaction output receives a new file id, but that file may contain logically older records. For this reason, restart loading sorts SSTables by their maximum record sequence number rather than by file id alone.

## 4. Data Freshness Rule

When the same key appears in multiple places, the newest version wins.

Priority order:

1. MemTable
2. Newer SSTable
3. Older SSTable

This is implemented using sequence numbers. File creation order is only a proxy before compaction; once older SSTables are merged into a new file, the compacted file may have a larger file id than an un-compacted newer SSTable.

## 5. MVP Invariants

The implementation should preserve these invariants:

- WAL is appended before MemTable update.
- SSTable files are immutable.
- MemTable entries are sorted before flush.
- Newer records override older records.
- Tombstone means deleted.
- Recovery rebuilds MemTable from WAL.
- SSTables are searched in freshness order.
- Corrupted WAL/SSTable lines are reported as errors instead of being silently ignored.

## 6. Limitations

- The line-based format does not escape tabs or newlines in keys and values.
- SSTable indexes are loaded fully into memory.
- Compaction is manual and single-threaded.
- There is no Bloom Filter or cache yet, so a missing or old key may require checking several SSTables.
- File replacement is simple and suitable for a course project, but not a fully crash-atomic production protocol.

## 7. Optional Optimizations

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
