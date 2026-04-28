#include "flashlsm/write_ahead_log.h"

#include <stdexcept>
#include <utility>
#include <fstream>

namespace flashlsm {

WriteAheadLog::WriteAheadLog(std::filesystem::path wal_path)
    : wal_path_(std::move(wal_path)) {}

void WriteAheadLog::append(const Record& record) {
    if(wal_path_.has_parent_path()){
        std::filesystem::create_directories(wal_path_.parent_path());
    }
    std::ofstream output(wal_path_, std::ios::app | std::ios::out);
    if(!output.is_open()){
        throw std::runtime_error("failed to open WAL file for appending");
    }
    output << record.sequence_number << '\t' 
            << (record.is_tombstone() ? "DEL" : "PUT") << '\t'
            << record.key << '\t' << record.value<<'\n';
    output.flush();
    if(!output){
        throw std::runtime_error("failed to write a record");
    }

}

std::vector<Record> WriteAheadLog::replay() const {
    std::vector<Record> records;

    if(!std::filesystem::exists(wal_path_)){
        return records;
    }

    std::ifstream input(wal_path_);
    if(!input.is_open()){
        throw std::runtime_error("failed to read WAL file for replaying");
    }
    std::string line;
    while(std::getline(input, line)){
        
        std::size_t first_tab = line.find('\t');
        std::size_t second_tab = line.find('\t', first_tab + 1);
        std::size_t third_tab = line.find('\t', second_tab + 1);

        if (first_tab == std::string::npos ||
            second_tab == std::string::npos ||
            third_tab == std::string::npos) {
            throw std::runtime_error("invalid WAL record format");
        }
        Record record;
        std::string seq_num_str, record_type_str, key, value;
        seq_num_str = line.substr(0, first_tab);
        record_type_str = line.substr(first_tab+1, second_tab - first_tab-1);
        key = line.substr(second_tab+1, third_tab - second_tab -1);
        value = line.substr(third_tab+1);

        record.key = key;
        record.value = value;
        record.sequence_number = std::stoull(seq_num_str);
        if(record_type_str == "PUT"){
            record.type = RecordType::Put;
        }
        else if(record_type_str == "DEL"){
            record.type = RecordType::Delete;
        }else{
            throw std::runtime_error("invalid record type");
        }
        records.push_back(record);
    }
    // 检测读取时出错
    if(!input.eof()){
        throw std::runtime_error("unknown error while reading wal file"); 
    }
    return records;
}

void WriteAheadLog::reset() {
    if (wal_path_.has_parent_path()) {
        std::filesystem::create_directories(wal_path_.parent_path());
    }
    // 直接覆盖写即可 把文件清空
    std::ofstream output(wal_path_, std::ios::out | std::ios::trunc);
    if (!output.is_open()) {
        throw std::runtime_error("failed to reset WAL file");
    }
}

const std::filesystem::path& WriteAheadLog::path() const {
    return wal_path_;
}

}  // namespace flashlsm
