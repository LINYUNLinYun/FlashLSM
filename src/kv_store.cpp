#include "flashlsm/kv_store.h"

#include <stdexcept>
#include <utility>

namespace flashlsm {

KVStore::KVStore(std::filesystem::path data_directory,
                 std::size_t memtable_flush_threshold_bytes)
    : data_directory_(std::move(data_directory)),
      memtable_flush_threshold_bytes_(memtable_flush_threshold_bytes),
      wal_(data_directory_ / "wal.log") {}

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
    memtable_.clear();
}

void KVStore::recover() {
    throw std::logic_error("KVStore::recover is not implemented");
}

void KVStore::load_sstables() {
    throw std::logic_error("KVStore::load_sstables is not implemented");
}

std::uint64_t KVStore::get_next_sequence_number() {
    return next_sequence_number_++;
}

void KVStore::write_record(const Record& record) {
    memtable_.put(record);
    if(memtable_.get_approximate_size_bytes() > memtable_flush_threshold_bytes_){
        flush();
    }
}

}  // namespace flashlsm
