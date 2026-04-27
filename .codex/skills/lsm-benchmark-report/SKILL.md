---
name: lsm-benchmark-report
description: Use when designing performance experiments, benchmarks, plots, or report sections for the LSM-Tree KV Store project, especially write throughput, read amplification, compaction, Bloom Filter, or LRU cache evaluation.
---

# LSM Benchmark and Report Skill

Use this skill when generating benchmark code, experiment plans, result analysis, or report text.

## Goal

Help convert implementation work into a clear experimental report.

## Recommended Benchmarks

### 1. Write Throughput

Measure number of `put()` operations per second.

Variables:

- MemTable size threshold
- value size
- number of records

Expected observation:

- larger MemTable may reduce flush frequency
- sequential append-oriented design should support high write throughput

### 2. Random Read Latency

Measure average time for `get()` on random keys.

Variables:

- number of SSTables
- with/without compaction
- with/without Bloom Filter
- with/without LRU cache

Expected observation:

- more SSTables increase read amplification
- compaction reduces number of files checked
- Bloom Filter avoids unnecessary SSTable lookup
- LRU improves repeated hot-key reads

### 3. Delete and Tombstone Cost

Measure read behavior after many deletes.

Expected observation:

- tombstones preserve correctness
- compaction is needed to clean stale records eventually

### 4. Before/After Compaction

Measure:

- SSTable count
- disk usage
- average files checked per read
- read latency

Expected observation:

- compaction reduces duplicated data and read amplification
- compaction may introduce write amplification

## Suggested Metrics

| Metric | Meaning |
|---|---|
| write throughput | writes per second |
| average read latency | average `get()` time |
| p95 read latency | tail read latency |
| SSTables checked per read | read amplification |
| disk usage | storage overhead |
| compaction time | maintenance cost |

## Report Writing Style

Prefer system-design explanation over user-manual writing.

Good:

> The write path first records updates in the WAL, then inserts them into the MemTable. This ordering ensures that data can be recovered after a crash before the MemTable is flushed.

Avoid:

> First run command X, then click Y.

## Report Structure

1. Motivation
2. LSM-Tree background
3. System design
4. Module design
5. Correctness mechanisms
6. Experiments
7. Analysis
8. Limitations
9. Future work

## AI Infra Connection

Optional paragraph:

> The project also reflects a common idea in AI infrastructure: hierarchical data management. LSM-Tree separates hot mutable data and cold immutable data, while inference systems such as KV-cache based serving engines also manage hot/cold memory layers and eviction policies.
