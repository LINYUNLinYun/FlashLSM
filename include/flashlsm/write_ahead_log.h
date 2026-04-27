#pragma once

#include <filesystem>
#include <vector>

#include "flashlsm/record.h"

namespace flashlsm {

// 追加写日志，必须先写 WAL，再更新 MemTable。
class WriteAheadLog {
public:
    explicit WriteAheadLog(std::filesystem::path wal_path);

    // 将记录持久化到 WAL，保证进程崩溃后仍可恢复。
    void append(const Record& record);

    // 按写入顺序回放 WAL 中的全部记录。
    std::vector<Record> replay() const;

    // 在成功 flush 之后清空或轮转 WAL。
    void reset();

    const std::filesystem::path& path() const;

private:
    std::filesystem::path wal_path_;
};

}  // namespace flashlsm
