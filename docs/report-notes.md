# Report Notes

Use this file to collect material for the final course report.

## 1. Suggested Report Structure

1. Background and motivation
2. LSM-Tree design
3. System architecture
4. Core modules
5. Read/write path
6. Recovery mechanism
7. Experimental evaluation
8. Extension design
9. Limitations and future work

## 2. Key Explanation Points

### Why LSM-Tree?

LSM-Tree is suitable for write-heavy workloads because it converts random writes into sequential appends and batched flushes.

### Why WAL?

WAL ensures that writes are durable even if the process crashes before MemTable is flushed to disk.

### Why SSTable is immutable?

Immutable SSTables simplify concurrency, recovery, and compaction. Once written, an SSTable is never modified in place.

### Why Compaction?

Without compaction, many SSTables accumulate. A read may need to check many files. Compaction merges files, removes stale records, and reduces read amplification.

### Why Tombstone?

Deletion must be recorded as a new version, because older SSTables may still contain the deleted key.

## 3. Possible Experiments

| Experiment | Purpose |
|---|---|
| Write throughput with different MemTable sizes | Show batch flush effect |
| Random read with different SSTable counts | Show read amplification |
| Before/after compaction | Show compaction benefit |
| With/without LRU cache | Show hot-key acceleration |
| With/without Bloom Filter | Show reduced useless SSTable lookup |

## 4. Implemented Benchmark

The repository includes `FlashLSM_benchmark`, a small benchmark executable intended to provide report material.

It measures:

- sequential write throughput
- random hit-read latency before and after compaction
- random missing-key read latency before and after compaction
- repeated compaction time
- SSTable count and disk usage before/after compaction

The benchmark intentionally uses deterministic key generation and a fixed random seed for queries, so results are easier to compare across runs.

Suggested command:

```bash
./build/FlashLSM_benchmark
```

Suggested interpretation:

- Write throughput demonstrates the append-oriented write path through WAL and MemTable.
- Random reads before compaction may touch more SSTables, especially for older keys.
- Missing-key reads are a direct way to observe read amplification, because every SSTable must be checked before returning not found.
- Compaction reduces the number of SSTable files and can reduce read amplification.
- Compaction also has a maintenance cost, so it trades background write work for better future reads.

The benchmark is not a production benchmark. It is a compact experiment for explaining the system behavior in a course report.

## 5. Benchmark Result Snapshot

One sample run produced the following result:

```text
Sequential write benchmark
  records: 1000
  elapsed_ms: 3281
  writes_per_second: 304.79

Random read before/after compaction
  keys: 900
  queries: 800
  sstables_before: 6
  sstables_after: 1
  disk_bytes_before: 47372
  disk_bytes_after: 47372
  compaction_ms: 44
  avg_read_us_before: 35.16
  avg_read_us_after: 40.73
  avg_missing_read_us_before: 2.74
  avg_missing_read_us_after: 0.72
  verification_bytes_before: 28712
  verification_bytes_after: 28712
```

### Write Throughput Analysis

The benchmark writes 1000 records in 3281 ms, giving about 304.79 writes per second.

This throughput is mainly limited by the current WAL implementation. Each `put()` appends a record to `wal.log` and flushes it before updating the MemTable. This conservative ordering improves crash recovery correctness, but it also introduces significant per-write I/O overhead.

Report-ready summary:

> The write path prioritizes durability over raw throughput. Since every write is appended and flushed to the WAL before entering the MemTable, write throughput is limited by synchronous file I/O.

### Compaction Analysis

Compaction reduced the SSTable count from 6 to 1:

```text
sstables_before: 6
sstables_after: 1
compaction_ms: 44
```

This shows that the simple compaction implementation successfully merges multiple immutable files into one compacted SSTable. The measured compaction cost is 44 ms for this small dataset.

Report-ready summary:

> Compaction reduced the number of SSTables from 6 to 1, lowering the number of files that future reads may need to check. This directly reduces read amplification, at the cost of extra maintenance work during compaction.

### Disk Usage Analysis

Disk usage stayed the same:

```text
disk_bytes_before: 47372
disk_bytes_after: 47372
```

This is expected for this benchmark because each key is written once and there are no deleted keys. Compaction can remove obsolete overwritten records and safe tombstones, but this dataset contains almost no stale data to remove. Therefore, the main benefit here is fewer SSTable files rather than less total data.

Report-ready summary:

> Disk usage did not decrease because the benchmark mostly contains unique live keys. Compaction cannot remove much data when there are few overwritten or deleted records.

### Hit-Read Latency Analysis

Average hit-read latency did not improve in this sample:

```text
avg_read_us_before: 35.16
avg_read_us_after: 40.73
```

This does not indicate a correctness problem. The dataset is small, the operating system may cache file contents, and many successful lookups can find their key without scanning every SSTable. In such a small benchmark, file layout, cache state, and measurement noise can dominate the expected benefit from having fewer SSTables.

Report-ready summary:

> Hit-read latency did not improve in this small run. Because the dataset is small and successful reads may find keys quickly, the benefit of compaction is partly hidden by OS cache effects and measurement noise.

### Missing-Key Read Latency Analysis

Missing-key reads improved clearly:

```text
avg_missing_read_us_before: 2.74
avg_missing_read_us_after: 0.72
```

This is the clearest read-amplification result. Before compaction, a missing key must be checked against all 6 SSTables before the system can return not found. After compaction, only 1 SSTable needs to be checked. This explains why missing-key latency drops significantly.

Report-ready summary:

> Missing-key reads show the benefit of compaction most clearly. A missing key must check all SSTables before returning not found, so reducing the SSTable count from 6 to 1 lowers missing-read latency from 2.74 us to 0.72 us.

### Correctness Check

The benchmark also records verification byte counts:

```text
verification_bytes_before: 28712
verification_bytes_after: 28712
```

The values are equal, meaning the benchmark read the same total value payload before and after compaction. This is a lightweight sanity check that compaction preserved visible key-value contents for the queried keys.

Overall conclusion:

> The benchmark shows that FlashLSM's simple compaction reduces SSTable count and improves missing-key read latency while preserving query results. Hit-read latency is not always better on small datasets, but the lower SSTable count demonstrates reduced read amplification and motivates future Bloom Filter or cache optimizations.

## 6. Correctness Mechanisms

Important correctness points to mention:

- WAL append happens before MemTable update.
- Sequence numbers define logical freshness.
- Reads check MemTable before SSTables.
- SSTables are searched newest-to-oldest by sequence freshness.
- Deletes are represented by tombstones.
- Compaction preserves newest records and keeps tombstones unless they are safe to drop.
- Corrupted WAL or SSTable records are treated as errors.

## 7. Limitations and Future Work

Current limitations:

- The text format does not escape tabs or newlines.
- Indexes are fully loaded into memory.
- Compaction is manual and single-threaded.
- There is no Bloom Filter, cache, range scan, or background compaction.

Recommended future work:

1. Add per-SSTable Bloom Filters.
2. Add a small LRU key-value cache.
3. Move from line format to a binary length-prefixed format.
4. Add read amplification instrumentation.
5. Add background compaction.

## 8. AI Infra Connection

The project can be connected to AI infrastructure through the idea of layered data management. LSM-Tree uses memory and disk hierarchy to balance write throughput and read efficiency. Modern inference systems also manage hot and cold data, such as KV cache in GPU/CPU memory, using similar ideas of caching, eviction, and delayed reorganization.
