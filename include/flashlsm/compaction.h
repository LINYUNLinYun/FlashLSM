#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

#include "flashlsm/sstable.h"

namespace flashlsm {

// 负责 SSTable compaction 给kvstore提供调用
class Compaction {
public:
    /**
     * @brief 构造一个 Compaction 上下文。
     *
     * @param data_directory 数据目录路径，新 SSTable 文件写入此目录
     * @param sstables       KVStore 的 SSTable 列表引用（newest-first 顺序）
     * @param next_sstable_id KVStore 的 SSTable ID 计数器引用，用于分配新文件名
     * @param max_compact_count 单次最多合并几个 SSTable，默认 4
     */
    Compaction(std::filesystem::path data_directory,
               std::vector<SSTable>& sstables,
               std::uint64_t& next_sstable_id,
               std::size_t max_compact_count = 4);

    /**
     * @brief 执行一次完整的 compaction 流程。
     *        如果可合并的 SSTable 不足 2 个，直接返回。
     *        成功后 sstables_ 列表和磁盘文件都会被更新。
     */
    void run();

private:
    /**
     * @brief 选择最老的几个做compaction
     * @return 被选中的 SSTable 在 sstables_ 中的索引列表（升序）
     */
    std::vector<std::size_t> select_sstables() const;

    /**
     * @brief 对选中的 SSTable 执行多路归并，输出一个新的合并 SSTable。
     *
     * 核心逻辑：
     *   1. 读取每个参与 SSTable 的全部记录
     *   2. 按 key 分组，同一 key 只保留 sequence_number 最大的那条记录
     *   3. 如果最新记录是 tombstone，且该 key 不存在于更老的（未参与合并的）SSTable 中，
     *      则可以安全丢弃；否则保留 tombstone 以遮盖旧值
     *   4. 将保留的记录写入一个新的 SSTable 文件
     *
     * @param compact_indices 参与合并的 SSTable 索引
     * @return 新创建的 SSTable 对象
     */
    SSTable merge(const std::vector<std::size_t>& compact_indices);

    /**
     * @brief 合并成功后，删除旧的 SSTable 文件并更新 sstables_ 列表。
     *
     * 步骤：
     *   1. 从 sstables_ 中移除已合并的条目
     *   2. 在 sstables_ 头部插入新的合并 SSTable（保持 newest-first 顺序）
     *   3. 从磁盘删除旧的 SSTable 文件（std::filesystem::remove）
     *
     * @param compact_indices 被替换的 SSTable 索引
     * @param new_table 合并后的新 SSTable
     */
    void cleanup(const std::vector<std::size_t>& compact_indices,
                 SSTable new_table);

    /**
     * @brief 检查某个 key 是否存在于"未参与合并的"更老 SSTable 中。
     *        用于判断 tombstone 是否可以安全丢弃。
     *
     * @param key 要检查的 key
     * @param compact_indices 参与合并的 SSTable 索引（这些会被跳过）
     * @return true 表示该 key 在更老的 SSTable 中有值，tombstone 不能丢
     */
    bool key_exists_in_older_sstables(const std::string& key,
                                      const std::vector<std::size_t>& compact_indices) const;

    std::filesystem::path data_directory_;
    std::vector<SSTable>& sstables_;         // 引用 KVStore 的列表，直接修改
    std::uint64_t& next_sstable_id_;         // 引用 KVStore 的 ID 计数器
    std::size_t max_compact_count_;
};

}  // namespace flashlsm
