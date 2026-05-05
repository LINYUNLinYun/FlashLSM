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

## 2. Shared Line Record Format

The current implementation uses the same line-based record encoding for WAL and SSTable data records:

```text
<seq>\t<type>\t<key>\t<value>\n
```

Example:

```text
1	PUT	user:1	Alice
2	PUT	user:2	Bob
3	DEL	user:1	
```

Fields:

| Field | Encoding |
|---|---|
| `seq` | Decimal `uint64_t` sequence number |
| `type` | `PUT` or `DEL` |
| `key` | Raw string without tab or newline escaping |
| `value` | Raw string without tab or newline escaping; empty for tombstone |

Current limitation: keys and values must not contain tab or newline characters, because the parser uses tabs as field separators and newline as record separator.

## 3. WAL Format

File name:

```text
wal.log
```

The WAL is append-only during normal writes. Every `put()` or `remove()` appends one line before the MemTable is updated. During recovery, the WAL is replayed in file order and records are inserted back into the MemTable.

After a successful flush, the WAL is reset to an empty file because the flushed records are now represented by an SSTable.

Invalid WAL lines cause replay to throw an error instead of being silently ignored.

## 4. SSTable Format

File name:

```text
sst_<id>.sst
```

Each SSTable is an immutable sorted text file. It stores one shared line-format record per key:

```text
<seq>\t<type>\t<key>\t<value>\n
```

SSTable records are written in key order because they are created from the ordered MemTable map or from compaction output.

When an SSTable is opened, the implementation scans the file once and builds an in-memory index:

```cpp
std::map<std::string, uint64_t> key_to_offset;
```

The index maps each key to the byte offset of its record line in the SSTable file. Point lookup uses the offset to seek directly to the record.

Invalid SSTable lines cause open or lookup to throw an error.

## 5. Sequence Numbers and Freshness

Each write receives a monotonically increasing sequence number. Larger sequence numbers represent newer logical versions.

The read path uses this freshness rule:

```text
MemTable first, then SSTables from newest to oldest.
```

After compaction, file id and logical freshness are not always the same. For example, a newly created compacted `sst_6.sst` may contain older records merged from `sst_1.sst` through `sst_4.sst`, while an un-compacted `sst_5.sst` may contain newer records. Startup loading therefore orders SSTables by their maximum contained sequence number, with file id only used as a tie-breaker.

## 6. Tombstone Rule

A tombstone record represents deletion.

During read:

- if the newest record is tombstone, return not found

During compaction:

- if a tombstone is the newest selected record and no older un-compacted SSTable can contain that key, the tombstone can be dropped
- otherwise, the tombstone must be preserved so it continues to hide older values

## 7. Possible Future Binary Format

A more robust implementation could replace the line format with a binary format that supports arbitrary bytes in keys and values:

```text
[seq:uint64][type:uint8][key_size:uint32][value_size:uint32][key][value]
```

That would require a storage-format migration and corresponding updates to SSTable/WAL parsing tests.
