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
    throw std::logic_error("KVStore::put is not implemented");
}

std::optional<std::string> KVStore::get(const std::string& key) {
    throw std::logic_error("KVStore::get is not implemented");
}

void KVStore::remove(const std::string& key) {
    throw std::logic_error("KVStore::remove is not implemented");
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

std::uint64_t KVStore::next_sequence_number() {
    throw std::logic_error("KVStore::next_sequence_number is not implemented");
}

void KVStore::write_record(Record record) {
    throw std::logic_error("KVStore::write_record is not implemented");
}

}  // namespace flashlsm
