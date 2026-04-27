#pragma once

#include <cstdint>
#include <string>

namespace flashlsm {

// 区分普通写入记录和逻辑删除记录。
enum class RecordType {
    Put,
    Delete,
};

// MemTable、WAL 和 SSTable 共用的统一记录结构。
// sequence_number 单调递增，数值越大表示版本越新。
struct Record {
    std::uint64_t sequence_number {0};
    RecordType type {RecordType::Put};
    std::string key;
    std::string value;

    // Tombstone 表示逻辑删除，旧版本数据暂时不会立即物理清除。
    bool is_tombstone() const;
};

}  // namespace flashlsm
