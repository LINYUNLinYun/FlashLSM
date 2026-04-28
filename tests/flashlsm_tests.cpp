#include <filesystem>
#include <string>
#include <system_error>

#include "flashlsm/kv_store.h"
#include "flashlsm/memtable.h"
#include "flashlsm/record.h"
#include "flashlsm/sstable.h"
#include "testkit.h"

namespace fs = std::filesystem;

namespace {

fs::path make_temp_dir(const std::string& name) {
    const fs::path dir = fs::temp_directory_path() / "flashlsm-tests" / name;
    std::error_code ec;
    fs::remove_all(dir, ec);
    fs::create_directories(dir);
    return dir;
}

flashlsm::Record make_put_record(std::uint64_t sequence_number,
                                 std::string key,
                                 std::string value) {
    flashlsm::Record record;
    record.sequence_number = sequence_number;
    record.type = flashlsm::RecordType::Put;
    record.key = std::move(key);
    record.value = std::move(value);
    return record;
}

flashlsm::Record make_delete_record(std::uint64_t sequence_number,
                                    std::string key) {
    flashlsm::Record record;
    record.sequence_number = sequence_number;
    record.type = flashlsm::RecordType::Delete;
    record.key = std::move(key);
    return record;
}

}  // namespace

int main() {
    return 0;
}

UnitTest(memtable_tracks_latest_value_and_size) {
    flashlsm::MemTable memtable;
    const flashlsm::Record first = make_put_record(1, "alpha", "one");
    const flashlsm::Record second = make_put_record(2, "alpha", "two");

    memtable.put(first);
    const std::size_t initial_size = memtable.get_approximate_size_bytes();
    memtable.put(second);

    const auto result = memtable.get("alpha");
    tk_assert(result.has_value(), "expected key to exist in memtable");
    tk_assert(result->value == "two", "expected latest value, got %s",
              result->value.c_str());
    tk_assert(memtable.get_entries().size() == 1,
              "expected only one live entry, got %zu",
              memtable.get_entries().size());
    tk_assert(memtable.get_approximate_size_bytes() >= initial_size,
              "expected approximate size to stay nondecreasing for overwrite");
}

UnitTest(memtable_clear_removes_entries) {
    flashlsm::MemTable memtable;
    memtable.put(make_put_record(1, "alpha", "one"));
    memtable.put(make_put_record(2, "beta", "two"));

    memtable.clear();

    tk_assert(memtable.empty(), "expected memtable to be empty after clear");
    tk_assert(memtable.get("alpha") == std::nullopt,
              "expected alpha to be removed");
    tk_assert(memtable.get_approximate_size_bytes() == 0,
              "expected approximate size to reset to 0");
}

UnitTest(sstable_round_trip_reads_put_and_delete_records) {
    const fs::path dir = make_temp_dir("sstable-round-trip");
    const fs::path table_path = dir / "table.sst";

    std::map<std::string, flashlsm::Record> entries;
    entries.emplace("alpha", make_put_record(3, "alpha", "value-a"));
    entries.emplace("beta", make_delete_record(4, "beta"));

    flashlsm::SSTable table =
        flashlsm::SSTable::create_from_memtable(table_path, entries);

    const auto alpha = table.get("alpha");
    const auto beta = table.get("beta");
    const auto gamma = table.get("gamma");

    tk_assert(alpha.has_value(), "expected alpha to be found");
    tk_assert(alpha->value == "value-a", "unexpected alpha value: %s",
              alpha->value.c_str());
    tk_assert(beta.has_value(), "expected beta tombstone to be found");
    tk_assert(beta->is_tombstone(), "expected beta to be a tombstone record");
    tk_assert(gamma == std::nullopt, "expected missing key to stay missing");
}

UnitTest(sstable_open_loads_index_for_existing_file) {
    const fs::path dir = make_temp_dir("sstable-open");
    const fs::path table_path = dir / "table.sst";

    std::map<std::string, flashlsm::Record> entries;
    entries.emplace("alpha", make_put_record(5, "alpha", "value-a"));
    entries.emplace("beta", make_put_record(6, "beta", "value-b"));
    flashlsm::SSTable::create_from_memtable(table_path, entries);

    flashlsm::SSTable reopened = flashlsm::SSTable::open(table_path);
    const auto beta = reopened.get("beta");

    tk_assert(beta.has_value(), "expected beta to be readable after open");
    tk_assert(beta->sequence_number == 6,
              "expected stored sequence number 6, got %llu",
              static_cast<unsigned long long>(beta->sequence_number));
    tk_assert(beta->value == "value-b", "unexpected beta value: %s",
              beta->value.c_str());
}

UnitTest(kvstore_put_get_remove_in_memtable) {
    const fs::path dir = make_temp_dir("kvstore-memtable");
    flashlsm::KVStore store(dir, 1 << 20);

    store.put("alpha", "one");
    store.put("alpha", "two");
    const auto before_remove = store.get("alpha");
    store.remove("alpha");
    const auto after_remove = store.get("alpha");

    tk_assert(before_remove.has_value(), "expected alpha before remove");
    tk_assert(before_remove.value() == "two",
              "expected latest in-memory value, got %s",
              before_remove->c_str());
    tk_assert(!after_remove.has_value(),
              "expected alpha to disappear after tombstone");
}

UnitTest(kvstore_flush_persists_and_serves_from_sstable) {
    const fs::path dir = make_temp_dir("kvstore-flush");
    flashlsm::KVStore store(dir, 1 << 20);

    store.put("alpha", "one");
    store.put("beta", "two");
    store.flush();

    const auto alpha = store.get("alpha");
    const auto beta = store.get("beta");
    const bool has_sstable = fs::exists(dir / "sst_1.sst");

    tk_assert(alpha.has_value(), "expected alpha after flush");
    tk_assert(beta.has_value(), "expected beta after flush");
    tk_assert(alpha.value() == "one", "unexpected alpha value after flush");
    tk_assert(beta.value() == "two", "unexpected beta value after flush");
    tk_assert(has_sstable, "expected flush to create sst_1.sst");
}
