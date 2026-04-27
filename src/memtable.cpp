#include "flashlsm/memtable.h"

#include <stdexcept>

namespace flashlsm {

void MemTable::put(const Record& record) {
    throw std::logic_error("MemTable::put is not implemented");
}

std::optional<Record> MemTable::get(const std::string& key) const {
    throw std::logic_error("MemTable::get is not implemented");
}

bool MemTable::empty() const {
    throw std::logic_error("MemTable::empty is not implemented");
}

std::size_t MemTable::approximate_size_bytes() const {
    throw std::logic_error("MemTable::approximate_size_bytes is not implemented");
}

void MemTable::clear() {
    throw std::logic_error("MemTable::clear is not implemented");
}

const std::map<std::string, Record>& MemTable::entries() const {
    throw std::logic_error("MemTable::entries is not implemented");
}

}  // namespace flashlsm
