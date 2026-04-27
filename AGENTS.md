# AGENTS.md

This repository is a course project for implementing a minimal LSM-Tree based KV Store.

## Project Goal

Build a small but complete LSM-Tree KV storage system with:

- `put(key, value)`
- `get(key)`
- `delete(key)`
- MemTable
- WAL
- SSTable
- flush
- recovery
- optional compaction, Bloom Filter, LRU cache, and benchmark

The implementation should prioritize correctness, readability, and explainability over industrial complexity.

## Development Rules

1. Keep the MVP simple before adding advanced features.
2. Do not introduce dependencies unless necessary.
3. Every storage format change must be documented in `docs/storage-format.md`.
4. Every feature must have at least one correctness test.
5. Prefer deterministic tests over random-only tests.
6. When modifying read/write path, update `docs/design.md`.
7. When adding benchmark results, update `docs/report-notes.md`.

## Suggested Tech Stack

- Language: C++17 or C++20
- Build: CMake
- Tests: simple executable tests or GoogleTest if already configured
- Storage: local filesystem files
- Serialization: custom binary format or simple line-based format for MVP

## MVP Scope

The MVP must include:

- In-memory ordered MemTable
- WAL append and recovery
- SSTable flush
- Basic SSTable lookup
- Tombstone deletion
- Read path: MemTable first, then SSTables from newest to oldest
- Basic test cases

## Optional Extension Priority

After MVP:

1. Simple compaction
2. Benchmark
3. Bloom Filter
4. LRU cache
5. Block-based SSTable
6. Background compaction thread

## Coding Style

- Use clear class boundaries.
- Avoid premature optimization.
- Prefer explicit names such as `MemTable`, `WriteAheadLog`, `SSTable`, `KVStore`.
- Use comments to explain storage invariants and file formats.
- Avoid large functions; split read path, write path, flush, and recovery logic.

## Safety Rules

- Never delete the entire data directory unless the test explicitly uses a temporary directory.
- Do not silently ignore corrupted WAL or SSTable records.
- Do not change public APIs without updating tests and docs.

## Useful Skills

Use the Codex skills in `.codex/skills/` when working on this project:

- `lsm-architecture`: design or refactor architecture
- `lsm-implementation`: implement MVP components
- `lsm-debugging`: debug correctness problems
- `lsm-benchmark-report`: design benchmarks and report analysis
