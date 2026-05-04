#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "flashlsm/compaction.h"
#include "flashlsm/memtable.h"
#include "flashlsm/sstable.h"
#include "flashlsm/write_ahead_log.h"

namespace flashlsm {

// 顶层协调器，负责组织 LSM 的写路径、读路径、flush 和恢复流程。
class KVStore {
public:
    explicit KVStore(std::filesystem::path data_directory,
                     std::size_t memtable_flush_threshold_bytes = 1024 * 1024);
    void put(const std::string& key, const std::string& value);     //插入
    std::optional<std::string> get(const std::string& key);

    /**
     * @brief 写入 tombstone 实现逻辑删除
     * 
     * @param key 
     */
    void remove(const std::string& key);

    void flush();

    /**
     * @brief 触发一次 compaction：选取若干老 SSTable，按 key 归并去重，
     *        合并为一个新的 SSTable，然后删除旧文件。
     *        如果没有可合并的 SSTable，直接返回。
     */
    void compact();

private:
    /**
     * @brief 重启时时根据WAL和SSTable 重建内存状态。
     * 
     */
    void recover();

    /**
     * @brief 重启时重建sstables
     * 
     */
    void load_sstables();

    // 统一分配递增序列号，保证所有写入都能比较新旧版本。
    std::uint64_t get_next_sequence_number();

    /**
     * @brief put/delete 共用的写路径辅助函数：分配序列号 -> 写 WAL -> 更新 MemTable -> 视情况触发 flush。
     * 
     * @param record 
     */
    void write_record(const Record& record);

    std::filesystem::path data_directory_;          // 数据目录路径 存sst文件
    std::size_t memtable_flush_threshold_bytes_;    // flush 阈值
    std::uint64_t next_sequence_number_ {1};        // 下一个序列号
    std::uint64_t next_sstable_id_ {1};             // 下一个 SSTable ID

    MemTable memtable_;                  // MemTable
    WriteAheadLog wal_;                 // 预写日志 
    std::vector<SSTable> sstables_;
};

}  // namespace flashlsm
