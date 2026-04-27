#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>

#include "flashlsm/record.h"

namespace flashlsm {

// 由 MemTable flush 生成的、不可变的有序磁盘表。
class SSTable {
public:
    SSTable() = default;
    explicit SSTable(std::filesystem::path table_path);

    // 根据已排序的 MemTable 内容构建新的 SSTable 文件。
    static SSTable create_from_memtable(
        const std::filesystem::path& table_path,
        const std::map<std::string, Record>& entries);

    // 打开已有 SSTable，并加载必要的内存元数据。
    static SSTable open(const std::filesystem::path& table_path);

    // 在当前 SSTable 中查找单个 key。
    std::optional<Record> get(const std::string& key) const;

    const std::filesystem::path& path() const;

private:
    // 维护 key 到磁盘偏移的映射，简化 MVP 阶段的点查。
    void load_index();

    std::filesystem::path table_path_;
    std::map<std::string, std::uint64_t> key_to_offset_;
};

}  // namespace flashlsm
