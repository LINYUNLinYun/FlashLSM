#include "flashlsm/kv_store.h"

#include <stdexcept>
#include <utility>
#include <iostream>
#include <algorithm>

namespace flashlsm {

KVStore::KVStore(std::filesystem::path data_directory,std::size_t memtable_flush_threshold_bytes)
    : data_directory_(std::move(data_directory)),
        memtable_flush_threshold_bytes_(memtable_flush_threshold_bytes),
        wal_(data_directory_ / "wal.log") {
            if(!std::filesystem::exists(data_directory_)){
                std::filesystem::create_directories(data_directory_);
            }
            // 载入 sstable
            load_sstables();
            // 恢复内存数据
            recover();
      }

void KVStore::put(const std::string& key, const std::string& value) {
    Record record;
    record.key = key;
    record.value = value;
    record.type = RecordType::Put;
    // 获取一个新的序列号
    record.sequence_number = get_next_sequence_number();
    write_record(record);
}

std::optional<std::string> KVStore::get(const std::string& key) {
    std::optional<flashlsm::Record> rec = memtable_.get(key);
    if(rec == std::nullopt){
        // 说明内存中没有 尝试去磁盘找
        for(const auto& sstable : sstables_){
            rec = sstable.get(key);
            // 这里要及时退出 因为我们只返回最新的value（从头插入从头开找）
            if(rec != std::nullopt){
                break;
            }
        }
    }
    if(rec == std::nullopt){
        // 磁盘也没找到
        return std::nullopt;
    }
    if(rec->is_tombstone()){
        return std::nullopt;
    }
    return rec->value;
    
}

void KVStore::remove(const std::string& key) {
    Record record;
    record.key = key;
    record.value = "";
    record.type = RecordType::Delete;
    // 获取一个新的序列号
    record.sequence_number = get_next_sequence_number();
    write_record(record);
}

void KVStore::flush() {
    if(memtable_.empty()){
        return;
    }
    // 获得sstable id 
    std::filesystem::path sst_path = data_directory_ / ("sst_" + std::to_string(next_sstable_id_++) + ".sst");
    // 磁盘化
    SSTable new_table = SSTable::create_from_memtable(sst_path, memtable_.get_entries());
    sstables_.insert(sstables_.begin(), new_table);
    wal_.reset();
    memtable_.clear();
}

void KVStore::recover() {
    const std::vector<Record> records = wal_.replay();

    // 恢复内存
    for(const auto r : records){
        memtable_.put(r);
        // 更新序列号
        if(r.sequence_number >= next_sequence_number_){
            next_sequence_number_ = r.sequence_number+1;

        }
    }
}

void KVStore::load_sstables() {
    sstables_.clear();
    if(!std::filesystem::exists(data_directory_)){
        // throw std::runtime_error("data directory unexist");
        return;
    }
    // 存一下id后面排序
    std::vector<std::pair<std::uint64_t, std::filesystem::path>> table_files;
    std::uint64_t max_sstable_id = 0;

    // 使用directory_iterator遍历目录下的文件
    for(const auto& entry : std::filesystem::directory_iterator(data_directory_)){
        if(!entry.is_regular_file()){
            // 排除非文件内容
            continue;
        }
        std::filesystem::path file_path = entry.path();
        const std::string prefix = "sst_";
        // 如果是文件 检查是不是.sst文件
        if(file_path.extension() == ".sst"){
            std::string filename = file_path.filename().string();
            // 检查名字是不是 sst_数字.sst这个格式 
            if(filename.substr(0,prefix.size()) == prefix){
                std::size_t dot_pos = filename.find('.');
                if(dot_pos == std::string::npos){
                    std::cerr << "skipped an invalid sstable file name: " << filename << std::endl;
                    continue;
                }
                std::string id_str = filename.substr(prefix.size(), dot_pos - prefix.size());

                uint64_t id = std::stoull(id_str);
                table_files.emplace_back(id, file_path);

                //更新下 id最大值 
                if (id > max_sstable_id) {
                    max_sstable_id = id;
                }
            }      
        }
    }

    // 排序 从大到小排序 因为我们采用的是从头插入的方式
    std::sort(table_files.begin(), table_files.end(), [](const auto& a, const auto& b){
        return a.first > b.first;
    });

    for(const auto& [id, path] : table_files){
        sstables_.emplace_back(SSTable::open(path));
    }

    // 更新next_sstable_id_
    next_sstable_id_ = max_sstable_id + 1;
}

std::uint64_t KVStore::get_next_sequence_number() {
    return next_sequence_number_++;
}

void KVStore::write_record(const Record& record) {
    wal_.append(record);
    memtable_.put(record);
    if(memtable_.get_approximate_size_bytes() > memtable_flush_threshold_bytes_){
        flush();
    }
}

void KVStore::compact() {
    Compaction compaction(data_directory_, sstables_, next_sstable_id_);
    compaction.run();
}

}  // namespace flashlsm
