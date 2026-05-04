#include "flashlsm/compaction.h"

#include <algorithm>
#include <map>
#include <stdexcept>
#include <iostream>

namespace flashlsm {

Compaction::Compaction(std::filesystem::path data_directory,
                       std::vector<SSTable>& sstables,
                       std::uint64_t& next_sstable_id,
                       std::size_t max_compact_count)
    : data_directory_(std::move(data_directory)),
      sstables_(sstables),
      next_sstable_id_(next_sstable_id),
      max_compact_count_(max_compact_count) {}

void Compaction::run() {
    // 选择最老的几个表
    std::vector<std::size_t> compact_indices = select_sstables();

    // 如果选中的 SSTable 不足 2 个，没有合并的必要
    if (compact_indices.size() < 2) {
        return;
    }

    // 合并
    SSTable new_table = merge(compact_indices);

    // 清除旧文件
    cleanup(compact_indices, std::move(new_table));
}

std::vector<std::size_t> Compaction::select_sstables() const {
    std::size_t max_selected_count = std::min(max_compact_count_, sstables_.size());
    if(max_selected_count < 2){
        std::cerr << "sstable files less than 2 \n";
        return {};
    }
    std::vector<std::size_t> selected;
    std::size_t n = sstables_.size();
    for(std::size_t i = 0; i < max_selected_count;i++){
        selected.push_back(n - i -1);
    }
    // 返回升序的数组 
    // std::sort(selected.begin(),selected.end());
    return selected;
}

SSTable Compaction::merge(const std::vector<std::size_t>& compact_indices) {
    if(compact_indices.empty()) {
        throw std::runtime_error("cannot merge empty compact indices");
    }
    std::map<std::string, Record> all_records;
    for(auto i : compact_indices){
        // SSTable temp = sstables_[i];
        std::vector<Record> records = sstables_[i].get_all_records();
        for(const auto& r : records){
            all_records[r.key] = r;
        }
    }
    std::map<std::string, Record> output_records;

    for(const auto& [key, record] : all_records){
        // 如果每次都只取最老的表合并那第二个条件是恒成立的 
        if(record.is_tombstone() && !key_exists_in_older_sstables(key, compact_indices)){
            continue;
        }
        output_records[key] = record;
    }
    std::filesystem::path new_path = data_directory_ / ("sst_" + std::to_string(next_sstable_id_++) + ".sst");
    return SSTable::create_from_memtable(new_path, output_records);
}

void Compaction::cleanup(const std::vector<std::size_t>& compact_indices,SSTable new_table){
    if(compact_indices.empty()){
        throw std::runtime_error("invalid compact indices while cleanup");
    }
    std::vector<std::filesystem::path> old_paths;
    // 先收集旧路径 再清空容器
    for(auto i : compact_indices){
        // 记录一下路径
        std::filesystem::path old_path = sstables_[i].path();
        old_paths.push_back(old_path);
    }
    // 清理容器 逻辑上erase前记录pos比较安全
    std::size_t insert_pos = compact_indices.back();
    for(auto i : compact_indices){
        sstables_.erase(sstables_.begin() + static_cast<std::ptrdiff_t>(i));
    }
    //先添加新文件 防止remove操作失败导致数据丢失
    sstables_.insert(sstables_.begin() + static_cast<std::ptrdiff_t>(insert_pos), std::move(new_table));
    // 删除源文件
    for(const auto& old_path : old_paths){
        std::filesystem::remove(old_path);
    }
    
    // sstables_.erase(sstables_.begin() + static_cast<std::ptrdiff_t>(i));
    // std::filesystem::remove(old_path);

}

bool Compaction::key_exists_in_older_sstables(const std::string& key, const std::vector<std::size_t>& compact_indices) const {
    if(compact_indices.empty()){
        throw std::runtime_error("invalid compact indices while finding keys in older sst files");
    }
    // 遍历sstables 由于要寻找最老的表 所以从尾部开始遍历
    // std::size_t n = sstables_.size();
    bool is_exist = false;
    std::size_t oldest = compact_indices.front();
    for(std::size_t i = oldest + 1; i < sstables_.size();i++){
        if(sstables_[i].get(key).has_value()){
            is_exist = true;
            break;
        }
    }
    return is_exist;
}

}  // namespace flashlsm
