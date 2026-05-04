#include "flashlsm/sstable.h"

#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace flashlsm {

SSTable::SSTable(std::filesystem::path table_path)
    : table_path_(std::move(table_path)) {}

SSTable SSTable::create_from_memtable(const std::filesystem::path& table_path, const std::map<std::string, Record>& entries) {
    // 确保父目录存在，避免写文件时失败。
    if (table_path.has_parent_path()) {
        std::filesystem::create_directories(table_path.parent_path());
    }
    // 使用标准 ofstream 以覆盖模式打开文件，确保每次 flush 都写入全新内容。
    std::ofstream output(table_path, std::ios::out | std::ios::trunc);
    if (!output.is_open()) {
        throw std::runtime_error("failed to open SSTable file for writing");
    }

    SSTable table(table_path);

    for (const auto& [key, record] : entries) {
        // 记录当前 key 对应行在文件中的起始写偏移。
        const std::streampos offset = output.tellp();
        if (offset == std::streampos(-1)) {
            throw std::runtime_error("failed to get SSTable write offset");
        }
        // offset要进行显示类型
        table.key_to_offset_[key] = static_cast<std::uint64_t>(offset);

        const char* record_type = record.is_tombstone() ? "DEL" : "PUT";
        // 按 line 格式写入：sequence_number \t record_type \t key \t value \n
        output << record.sequence_number
               << '\t'
               << record_type
               << '\t'
               << record.key
               << '\t'
               << record.value
               << '\n';

        if (!output) {
            throw std::runtime_error("failed to write SSTable record");
        }
    }
    // 输出文件流缓冲区刷进文件里 要确保成功
    output.flush();
    if (!output) {
        throw std::runtime_error("failed to flush SSTable file");
    }

    return table;
}

SSTable SSTable::open(const std::filesystem::path& table_path) {
    SSTable table(table_path);
    table.load_index();
    return table;

}

std::optional<Record> SSTable::get(const std::string& key) const {
    // if(key_to_offset_.empty()){
    //     return std::nullopt;
    // }
    auto it = key_to_offset_.find(key);
    if(it == key_to_offset_.end()){
        return std::nullopt;
    }
    uint64_t offset = it->second;

    std::ifstream input(table_path_);
    if(!input.is_open()){
        throw std::runtime_error("failed to open SSTable file for reading");
    }
    // 移动文件指针
    input.seekg(static_cast<std::streampos>(offset));
    if(!input){
        throw std::runtime_error("failed to seek to offset in SSTable file");
    }
    // 解析行内容
    std::string line;
    if(!std::getline(input, line)){
        throw std::runtime_error("failed to read line from SSTable file");
    }
    std::size_t first_tab = line.find('\t');
    if(first_tab == std::string::npos){
        throw std::runtime_error("invalid record format");
    }
    std::size_t second_tab = line.find('\t', first_tab+1);
    if(second_tab == std::string::npos){
        throw std::runtime_error("invalid record format");
    }
    std::size_t third_tab = line.find('\t', second_tab+1);
    if(third_tab == std::string::npos){
        throw std::runtime_error("invalid record format");
    }
    // 新建一个记录
    Record record;
    std::string key_str, value_str;
    std::string type_str;
    std::string seq_num_str;
    seq_num_str = line.substr(0, first_tab);
    type_str = line.substr(first_tab+1, second_tab-first_tab-1);
    key_str = line.substr(second_tab+1, third_tab-second_tab-1);
    value_str = line.substr(third_tab+1);
    
    if(type_str == "PUT"){
        record.type = RecordType::Put;
    }else if(type_str == "DEL"){
        record.type = RecordType::Delete;
    }else{
        throw std::runtime_error("invalid record type in SSTable file");
    }
    record.key = key_str;
    record.value = value_str;
    record.sequence_number = std::stoull(seq_num_str);

    return record;
    
}

const std::filesystem::path& SSTable::path() const {
    return table_path_;
}

void SSTable::load_index() {
    if(!key_to_offset_.empty()){
        throw std::logic_error("Private method, SSTable::load_index should only be called once per SSTable instance");
        return;
    }
    std::ifstream input(table_path_);
    if (!input.is_open()) {
        throw std::runtime_error("failed to open SSTable file for reading");
        return;
    }
    std::string line;
    while(true){
        // 找一下读offset在哪
        const std::streampos offset = input.tellg();
        if(!std::getline(input, line)){
            // 文件读完了退出
            if(input.eof()){
                break;
            }
            throw std::runtime_error("failed to read line when loading index");
        }
        // offset检查要排除文件可能读完的情况 所以放在getline检查后面 
        if (offset == std::streampos(-1)) {
            throw std::runtime_error("failed to get SSTable read offset");
            break;
        }
        // const char* record_type;
        // uint64_t seq_num;
        std::string key;
        std::size_t first_tab = line.find('\t');
        std::size_t second_tab = line.find('\t', first_tab+1);
        std::size_t third_tab = line.find('\t', second_tab+1);

        if(first_tab == std::string::npos || second_tab == std::string::npos || third_tab == std::string::npos){
            throw std::runtime_error("read a wrong line when loading index: invalid record format");
        }
        key = line.substr(second_tab+1,third_tab-second_tab -1);
        key_to_offset_[key] = static_cast<std::uint64_t>(offset);
    }
    

}

std::uint64_t SSTable::id() const {
    std::filesystem::path file_path = table_path_;
    const std::string prefix = "sst_";
    // 如果是文件 检查是不是.sst文件
    if(file_path.extension() == ".sst"){
        std::string filename = file_path.filename().string();
        // 检查名字是不是 sst_数字.sst这个格式 
        if(filename.substr(0,prefix.size()) == prefix){
            std::size_t dot_pos = filename.find('.');
            if(dot_pos == std::string::npos){
                throw std::runtime_error("invalid sstable file name: " + filename);
            }
            std::string id_str = filename.substr(prefix.size(), dot_pos - prefix.size());
            if(id_str.empty()){
                throw std::runtime_error("invalid sstable file name: " + filename);
            } 
            uint64_t id = std::stoull(id_str);
            return id;
        }      
    }
    return 0;
}

std::vector<Record> SSTable::get_all_records() const {
    std::vector<Record> records;
    std::ifstream input(table_path_);
    if(!input.is_open()){
        throw std::runtime_error("failed to open SSTable file for reading all records");
    }
    for(const auto& [key, offset] : key_to_offset_){
        // 读指针移动
        input.seekg(static_cast<std::streampos>(offset));
        if(!input){
            throw std::runtime_error("failed to seek offset in .sst");
        }
        std::string line;
        if(!std::getline(input, line)){
            throw std::runtime_error("failed to read line from SSTable file");
        }

        std::size_t first_tab = line.find('\t');
        std::size_t second_tab = line.find('\t', first_tab + 1);
        std::size_t third_tab = line.find('\t', second_tab + 1);

        if (first_tab == std::string::npos ||
            second_tab == std::string::npos ||
            third_tab == std::string::npos) {
            throw std::runtime_error("invalid record format");
        }

        Record record;
        record.sequence_number = std::stoull(line.substr(0, first_tab));

        std::string type_str = line.substr(first_tab + 1, second_tab - first_tab - 1);
        if (type_str == "PUT") {
            record.type = RecordType::Put;
        } else if (type_str == "DEL") {
            record.type = RecordType::Delete;
        } else {
            throw std::runtime_error("invalid record type in SSTable file");
        }

        record.key = line.substr(second_tab + 1, third_tab - second_tab - 1);
        record.value = line.substr(third_tab + 1);

        records.push_back(record);
    }

    
    return records;
}

}  // namespace flashlsm
