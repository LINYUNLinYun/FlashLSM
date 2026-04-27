---
name: lsm-architecture
description: Use when designing, reviewing, or refactoring the LSM-Tree KV Store architecture, including MemTable, WAL, SSTable, flush, compaction, cache, and module boundaries.
---

# LSM Architecture Skill

Use this skill when the user asks to design, restructure, review, or explain the storage engine architecture.

## Goal

Help maintain a clean LSM-Tree architecture that is simple enough for a course project but close enough to real systems to be meaningful.

## Required Output

When invoked, produce one or more of:

- module breakdown
- data flow diagram in text form
- class responsibility list
- invariants
- trade-off analysis
- implementation plan

## Architecture Principles

1. Keep the MVP first.
2. Separate write path, read path, persistence, and maintenance tasks.
3. Keep file formats documented.
4. Treat SSTables as immutable.
5. Make data freshness explicit: newest record wins.
6. Use tombstones for deletes.
7. Do not introduce compaction before basic flush/get is correct.

## Recommended Module Boundaries

### KVStore

Coordinates all components.

Responsibilities:

- expose public API
- assign sequence numbers
- call WAL before MemTable update
- trigger flush
- choose read search order

### MemTable

Responsibilities:

- maintain sorted in-memory records
- support lookup
- support iteration for flush
- track approximate size

### WriteAheadLog

Responsibilities:

- append PUT and DEL records
- flush durable records
- replay records during recovery
- rotate or clear after successful flush

### SSTable

Responsibilities:

- write sorted immutable file
- load or read index
- lookup key
- optionally include Bloom Filter

### Compactor

Responsibilities:

- select SSTables
- merge sorted records
- keep newest version
- preserve tombstone correctness
- atomically install compacted output

### Cache

Responsibilities:

- cache hot key-value pairs or blocks
- use LRU eviction
- avoid caching tombstones unless explicitly designed

## Common Architecture Review Checklist

- Does every write go to WAL before MemTable?
- Can the system recover after crash?
- Does read path check newest data first?
- Are tombstones handled correctly?
- Are SSTables immutable?
- Is file deletion atomic and safe?
- Are tests updated when architecture changes?

## Avoid

- Building a distributed system.
- Adding concurrency before correctness.
- Mixing file parsing logic into `KVStore`.
- Deleting tombstones too early.
- Implementing block cache before simple key-value cache unless explicitly requested.
