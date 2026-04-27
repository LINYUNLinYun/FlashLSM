---
name: lsm-implementation
description: Use when implementing MVP LSM-Tree KV Store features: put, get, delete, MemTable, WAL, recovery, SSTable flush, SSTable lookup, and basic tests.
---

# LSM Implementation Skill

Use this skill when writing or modifying actual source code for the LSM KV Store.

## Goal

Implement the smallest correct version first, then add extensions.

## Implementation Order

Follow this order unless the user explicitly asks otherwise:

1. `KVStore` interface
2. `MemTable`
3. WAL append
4. WAL recovery
5. SSTable writer
6. SSTable reader
7. flush trigger
8. tombstone delete
9. integration tests
10. simple compaction

## MVP API

Prefer an API like:

```cpp
class KVStore {
public:
    explicit KVStore(const std::string& data_dir);

    void put(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key);
    void remove(const std::string& key);

    void flush();
};
```

## Record Model

Use a unified record structure:

```cpp
enum class RecordType {
    Put,
    Delete
};

struct Record {
    uint64_t sequence_number;
    RecordType type;
    std::string key;
    std::string value;
};
```

## Correct Write Path

Every write must follow this order:

1. allocate sequence number
2. append to WAL
3. insert into MemTable
4. maybe flush

Never update MemTable before WAL append succeeds.

## Correct Read Path

Read order:

1. MemTable
2. newest SSTable to oldest SSTable

If tombstone is found, return `std::nullopt`.

## Flush Rules

During flush:

1. write MemTable entries to a temporary SSTable file
2. fsync/close file if implemented
3. rename temp file to final file
4. add SSTable to manifest or in-memory list
5. clear MemTable
6. rotate WAL

## Testing Requirements

Add tests for:

- put then get
- overwrite key
- delete existing key
- delete missing key
- flush then get
- restart recovery from WAL
- newer value in MemTable overrides older SSTable
- tombstone overrides older value

## Simplicity Preference

For a course MVP, it is acceptable to:

- use `std::map` for MemTable
- keep SSTable index in memory
- use one WAL file
- use synchronous flush
- use single-threaded compaction

## Avoid

- Complex background threads before tests pass.
- Custom memory management.
- Over-abstracted class hierarchies.
- Undocumented binary format changes.
