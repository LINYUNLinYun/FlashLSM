#include "flashlsm/write_ahead_log.h"

#include <stdexcept>
#include <utility>

namespace flashlsm {

WriteAheadLog::WriteAheadLog(std::filesystem::path wal_path)
    : wal_path_(std::move(wal_path)) {}

void WriteAheadLog::append(const Record& record) {
    throw std::logic_error("WriteAheadLog::append is not implemented");
}

std::vector<Record> WriteAheadLog::replay() const {
    throw std::logic_error("WriteAheadLog::replay is not implemented");
}

void WriteAheadLog::reset() {
    throw std::logic_error("WriteAheadLog::reset is not implemented");
}

const std::filesystem::path& WriteAheadLog::path() const {
    return wal_path_;
}

}  // namespace flashlsm
