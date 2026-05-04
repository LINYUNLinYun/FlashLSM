#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "flashlsm/record.h"

namespace flashlsm {

// 由 MemTable flush 生成的、不可变的有序磁盘表。
class SSTable {
public:
    SSTable() = default;
    // 禁止隐式转换
    explicit SSTable(std::filesystem::path table_path);

    /**
     * @brief 静态方法，从MemTable构建一个SSTable文件，该方法确保成功否则抛出异常。
     * 
     * @param table_path 路径
     * @param entries memtable 记录
     * @return SSTable 返回新的sstable对象
     */
    static SSTable create_from_memtable(
        const std::filesystem::path& table_path,
        const std::map<std::string, Record>& entries);

    // 打开已有 SSTable，并加载必要的内存元数据。
    static SSTable open(const std::filesystem::path& table_path);

    /**
     * @brief 只读操作 在当前SSTable 中查找单个key。
     * 
     * @param key 
     * @return std::optional<Record> 
     */
    std::optional<Record> get(const std::string& key) const;

    const std::filesystem::path& path() const;

    // 获取ID（table path解析）。
    std::uint64_t id() const;

    /**
     * @brief 按 key 顺序读取 SSTable 中的全部记录。
     * 用于 compaction 时的多路归并，返回的 vector 已按 key 排序。
     */
    std::vector<Record> get_all_records() const;

private:
    // 维护 key 到磁盘偏移的映射，简化 MVP 阶段的点查。
    void load_index();

    std::filesystem::path table_path_;
    std::map<std::string, std::uint64_t> key_to_offset_;
};

}  // namespace flashlsm
