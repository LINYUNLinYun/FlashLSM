#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

#include "flashlsm/sstable.h"

namespace flashlsm {

// 负责SSTable compact 给kvstore提供调用
class Compaction {
public:
    /**
     * @brief 构造一个 Compaction 上下文。
     *
     * @param data_directory 数据目录路径
     * @param sstables       KVStore 的 SSTable 列表引用
     * @param next_sstable_id KVStore 的 SSTable ID 计数器引用
     * @param max_compact_count 单次最多合并几个 SSTable，默认 4
     */
    Compaction(std::filesystem::path data_directory,
               std::vector<SSTable>& sstables,
               std::uint64_t& next_sstable_id,
               std::size_t max_compact_count = 4);

    /**
     * @brief 执行一次完整的compaction流程。成功后sstables_和磁盘文件都会被更新。
     */
    void run();

private:
    /**
     * @brief 选择最老的几个做compaction
     * @return 被选中的 SSTable 在 sstables_ 中的索引列表（降序）
     */
    std::vector<std::size_t> select_sstables() const;

    /**
     * @brief 对选中的 SSTable 执行多路归并，输出一个新的合并 SSTable。
     *
     * 需要注意的是，这一版只用了新覆盖旧，没有进行最新的seq num比较，因此一定要确保sstable的新旧关系合法
     *
     * @param compact_indices 参与合并的 SSTable 索引
     * @return 新创建的SSTable 对象
     */
    SSTable merge(const std::vector<std::size_t>& compact_indices);

    /**
     * @brief 合并成功后，删除旧的SSTable文件并更新sstables_ 列表。
     *   会执行容器修改——删除sstable_中被合并的旧表，插入新的合并表(在尾部插入)
     *   会删除磁盘文件——删除被合并的旧表对应的文件
     *
     * @param compact_indices 被替换的 SSTable 索引
     * @param new_table 合并后的新 SSTable
     */
    void cleanup(const std::vector<std::size_t>& compact_indices,
                 SSTable new_table);

    /**
     * @brief 当前版本其实并不启用
     */
    bool key_exists_in_older_sstables(const std::string& key,
                                      const std::vector<std::size_t>& compact_indices) const;

    std::filesystem::path data_directory_;
    std::vector<SSTable>& sstables_;         // 引用 KVStore 的列表，直接修改
    std::uint64_t& next_sstable_id_;         // 引用 KVStore 的 ID 计数器
    std::size_t max_compact_count_;
};

}  // namespace flashlsm
