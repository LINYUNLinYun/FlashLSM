#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "flashlsm/memtable.h"
#include "flashlsm/sstable.h"
#include "flashlsm/write_ahead_log.h"

namespace flashlsm {

// 顶层协调器，负责组织 LSM 的写路径、读路径、flush 和恢复流程。
class KVStore {
public:
    explicit KVStore(std::filesystem::path data_directory,
                     std::size_t memtable_flush_threshold_bytes = 1024 * 1024);
    void put(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key);

    /**
     * @brief 写入 tombstone 实现逻辑删除
     * 
     * @param key 
     */
    void remove(const std::string& key);

    // 手动触发 flush，便于测试和受控实验。
    void flush();

private:
    // 启动时根据现有 WAL 和 SSTable 重建内存状态。
    void recover();

    // 扫描数据目录中的 SSTable，并按“从新到旧”顺序组织读路径。
    void load_sstables();

    // 统一分配递增序列号，保证所有写入都能比较新旧版本。
    std::uint64_t get_next_sequence_number();

    // put/delete 共用的写路径辅助函数：
    // 分配序列号 -> 写 WAL -> 更新 MemTable -> 视情况触发 flush。
    void write_record(Record record);

    std::filesystem::path data_directory_;          // 数据目录路径
    std::size_t memtable_flush_threshold_bytes_;    // flush 阈值
    std::uint64_t next_sequence_number_ {1};        // 下一个序列号
    std::uint64_t next_sstable_id_ {1};             // 下一个 SSTable ID

    MemTable memtable_;                  // MemTable
    WriteAheadLog wal_;
    std::vector<SSTable> sstables_;
};

}  // namespace flashlsm
