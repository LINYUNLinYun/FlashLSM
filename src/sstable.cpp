#include "flashlsm/sstable.h"

#include <stdexcept>
#include <utility>

namespace flashlsm {

SSTable::SSTable(std::filesystem::path table_path)
    : table_path_(std::move(table_path)) {}

SSTable SSTable::create_from_memtable(
    const std::filesystem::path& table_path,
    const std::map<std::string, Record>& entries) {
    throw std::logic_error("SSTable::create_from_memtable is not implemented");
}

SSTable SSTable::open(const std::filesystem::path& table_path) {
    throw std::logic_error("SSTable::open is not implemented");
}

std::optional<Record> SSTable::get(const std::string& key) const {
    throw std::logic_error("SSTable::get is not implemented");
}

const std::filesystem::path& SSTable::path() const {
    return table_path_;
}

void SSTable::load_index() {
    throw std::logic_error("SSTable::load_index is not implemented");
}

}  // namespace flashlsm
