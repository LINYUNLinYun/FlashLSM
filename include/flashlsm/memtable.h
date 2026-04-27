#pragma once

#include <cstddef>
#include <map>
#include <optional>
#include <string>

#include "flashlsm/record.h"

namespace flashlsm {

// 有序内存表，保存每个 key 当前最新、尚未 flush 的版本。
class MemTable {
public:
    MemTable() = default;

    // 插入一条记录；如果 key 已存在，则覆盖为更新版本。
    void put(const Record& record);

    // 查询 key 在内存中的最新记录；不存在则返回空。
    std::optional<Record> get(const std::string& key) const;

    bool empty() const;

    // 返回近似内存占用，用于判断是否触发 flush。
    std::size_t approximate_size_bytes() const;

    void clear();

    // 暴露有序内容，供 flush 阶段顺序写入 SSTable。
    const std::map<std::string, Record>& entries() const;

private:
    std::map<std::string, Record> entries_;
    std::size_t approximate_size_bytes_ {0};
};

}  // namespace flashlsm
