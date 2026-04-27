---
name: lsm-debugging
description: Use when debugging LSM-Tree correctness issues such as missing keys, stale reads, failed recovery, tombstone errors, SSTable search bugs, or compaction data loss.
---

# LSM Debugging Skill

Use this skill when a test fails or the user reports incorrect KV behavior.

## Debugging Strategy

Start from the failing observable behavior:

- Is the key missing?
- Is an old value returned?
- Does delete fail?
- Does recovery fail?
- Does compaction lose data?

Then trace the key through the write/read path.

## Key Trace Checklist

For a given key, inspect:

1. WAL records
2. MemTable state
3. SSTable files from newest to oldest
4. tombstone records
5. compaction output

## Common Bugs

### Bug 1: Old Value Returned

Likely causes:

- SSTables searched oldest to newest
- sequence number not considered
- MemTable not checked before SSTables
- compaction kept older version

Fix:

- search newest data first
- preserve sequence numbers
- during merge, keep highest sequence number

### Bug 2: Delete Does Not Work

Likely causes:

- delete physically removed key from MemTable only
- tombstone not flushed
- get ignores tombstone
- compaction dropped tombstone too early

Fix:

- represent delete as a tombstone record
- return not found when tombstone is newest record

### Bug 3: Data Lost After Restart

Likely causes:

- WAL append after MemTable update
- WAL not flushed
- recovery not replaying delete records
- WAL cleared before SSTable safely created

Fix:

- append WAL first
- rotate WAL only after successful flush

### Bug 4: Flush Produces Unreadable SSTable

Likely causes:

- inconsistent storage format
- wrong offsets in index
- missing footer/index metadata
- newline escaping issue in line-based format

Fix:

- write and read using one shared format helper
- add a test that writes SSTable then reads every key back

### Bug 5: Compaction Loses Latest Value

Likely causes:

- merging by key without comparing sequence
- input SSTables processed in wrong order
- tombstone dropped incorrectly

Fix:

- for each key, keep record with largest sequence number
- keep tombstones unless safe deletion is proven

## Minimal Debug Output

When debugging, add temporary logs like:

```text
TRACE key=<key> source=<memtable|sstable> seq=<seq> type=<put|delete>
```

## Debugging Tests to Add

- overwrite before flush
- overwrite after flush
- delete before flush
- delete after flush
- compact two files containing same key
- recover WAL containing put and delete
