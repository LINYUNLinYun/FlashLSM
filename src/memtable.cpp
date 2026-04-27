#include "flashlsm/memtable.h"

#include <stdexcept>

namespace flashlsm {


void MemTable::put(const Record& record) {
    auto it = entries_.find(record.key);
    if(it == entries_.end()){
        // 新纪录 插入
        // entries_[record.key] = record;
        entries_.insert({record.key, record});
        approximate_size_bytes_ += sizeof(Record)+record.key.size()+record.value.size();
    }else{
        // 旧纪录 
        approximate_size_bytes_ -= sizeof(Record)+it->second.key.size()+it->second.value.size();
        entries_[record.key] = record;
        approximate_size_bytes_ += sizeof(Record)+record.key.size()+record.value.size();
    }
    return;
}

std::optional<Record> MemTable::get(const std::string& key) const {
    auto it = entries_.find(key);
    if(it == entries_.end()){
        return std::nullopt;
    }else{
        return it->second;
    }
}

bool MemTable::empty() const {
    return entries_.empty();
}

std::size_t MemTable::get_approximate_size_bytes() const {
    return approximate_size_bytes_;
}

void MemTable::clear() {
    entries_.clear();
    approximate_size_bytes_ = 0;
}

const std::map<std::string, Record>& MemTable::get_entries() const {
    return entries_;
}

}  // namespace flashlsm
