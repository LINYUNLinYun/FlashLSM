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
    memtable_.put(record);
}

std::optional<std::string> KVStore::get(const std::string& key) {
    std::optional<flashlsm::Record> rec = memtable_.get(key);
    if(!rec){
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
    memtable_.put(record);
}

void KVStore::flush() {
    throw std::logic_error("KVStore::flush is not implemented");
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

void KVStore::write_record(Record record) {
    throw std::logic_error("KVStore::write_record is not implemented");
}

}  // namespace flashlsm
