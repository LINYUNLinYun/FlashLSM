# Storage Format

This document records the on-disk format used by WAL and SSTable files.

## 1. Record Model

Each logical record contains:

| Field | Meaning |
|---|---|
| sequence_number | Monotonic version number |
| record_type | value or tombstone |
| key | string key |
| value | string value; empty for tombstone |

## 2. WAL Format

MVP can use a simple line-based format:

```text
<seq>\t<type>\t<key>\t<value>\n
```

Example:

```text
1	PUT	user:1	Alice
2	PUT	user:2	Bob
3	DEL	user:1	
```

For a more robust binary format:

```text
[seq:uint64][type:uint8][key_size:uint32][value_size:uint32][key][value]
```

## 3. SSTable Format

Recommended MVP binary format:

```text
Header
Data Section
Index Section
Footer
```

### 3.1 Header

```text
[magic:uint32][version:uint32][entry_count:uint64]
```

### 3.2 Data Record

```text
[seq:uint64][type:uint8][key_size:uint32][value_size:uint32][key][value]
```

### 3.3 Index Entry

```text
[key_size:uint32][key][offset:uint64]
```

### 3.4 Footer

```text
[index_offset:uint64][index_count:uint64]
```

## 4. Simplified MVP Alternative

If time is limited, use one sorted line-based SSTable:

```text
<seq>\t<type>\t<key>\t<value>\n
```

Then load a full in-memory index at startup:

```cpp
std::map<std::string, uint64_t> key_to_offset;
```

## 5. Tombstone Rule

A tombstone record represents deletion.

During read:

- if newest record is tombstone, return not found

During compaction:

- if a tombstone is older than all lower-level data for that key, it can be dropped
- for MVP simple compaction, keep tombstones unless you are sure they are safe to remove
