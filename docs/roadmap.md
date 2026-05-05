# Development Roadmap

## Phase 1: MVP Correctness [DONE]

Goal: produce a complete minimal LSM KV Store.

Tasks:

1. [x] Create `KVStore` public API.
2. [x] Implement `MemTable` with `std::map`.
3. [x] Implement WAL append.
4. [x] Implement WAL recovery.
5. [x] Implement flush from MemTable to SSTable.
6. [x] Implement SSTable lookup.
7. [x] Implement tombstone deletion.
8. [x] Add tests.

Expected result:

- `put/get/delete` work.
- Data survives process restart if WAL exists.
- Data survives flush to SSTable.

## Phase 2: Simple Compaction [DONE]

Goal: reduce duplicate entries across SSTables.

Tasks:

1. [x] 搭建框架：在 KVStore 和 SSTable 中声明 compaction 相关方法
2. [x] 实现 `SSTable::id()` —— 从文件名解析 SSTable 数字 ID
3. [x] 实现 `SSTable::get_all_records()` —— 读取 SSTable 全部记录
4. [x] 实现 `Compaction::select_sstables()` —— 选取最老的若干 SSTable
5. [x] 实现 `Compaction::merge()` —— 按 key 去重保留最新记录，处理 tombstone 安全性
6. [x] 实现 `Compaction::cleanup()` —— 替换旧表、删除旧文件
7. [x] 实现 `KVStore::compact()` —— 串联 compaction 流程
8. [x] 编写 compaction 测试用例

Expected result:

- Fewer SSTables.
- Reads become faster.
- Old versions are cleaned.

## Phase 3: Benchmark [DONE - BASIC]

Goal: quantify system behavior.

Tasks:

1. [x] Sequential write benchmark.
2. [x] Random read benchmark.
3. [ ] Read-after-write benchmark.
4. [x] Compare before and after compaction.
5. [ ] Optionally compare with and without cache.

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

Recommended next optional feature: Bloom Filter per SSTable. It directly targets read amplification and is easy to explain in the final report.

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
