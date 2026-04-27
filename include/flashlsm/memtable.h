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
    MemTable() = default;       // 默认构造函数

    /**
     * @brief 插入一条记录；如果 key 已存在，则覆盖为更新版本。
     * 
     * @param record 
     */
    void put(const Record& record);

    /**
     * @brief 查询 key 在内存中的最新记录；不存在则返回空。
     * 
     * @param key 
     * @return std::optional<Record> 空时返回std::nullopt
     */
    std::optional<Record> get(const std::string& key) const;

    bool empty() const;

    /**
     * @brief 返回近似内存占用，用于判断是否触发 flush。
     * 
     * @return std::size_t 
     */
    std::size_t get_approximate_size_bytes() const;

    void clear();

    /**
     * @brief // 对外的接口，供 flush 阶段顺序写入 SSTable。
     * 
     * @return const std::map<std::string, Record>& 返回常量引用
     */
    const std::map<std::string, Record>& get_entries() const;

private:
    std::map<std::string, Record> entries_;
    std::size_t approximate_size_bytes_ {0};
};

}  // namespace flashlsm
