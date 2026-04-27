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

## 4. AI Infra Connection

The project can be connected to AI infrastructure through the idea of layered data management. LSM-Tree uses memory and disk hierarchy to balance write throughput and read efficiency. Modern inference systems also manage hot and cold data, such as KV cache in GPU/CPU memory, using similar ideas of caching, eviction, and delayed reorganization.
