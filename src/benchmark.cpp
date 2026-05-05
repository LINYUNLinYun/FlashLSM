#include "flashlsm/kv_store.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

struct ReadResult {
    double average_microseconds;
    std::size_t found_count;
    std::size_t value_bytes;
};

std::string make_key(std::size_t id) {
    return "key-" + std::to_string(id);
}

std::string make_value(std::size_t id) {
    return "value-" + std::to_string(id) + "-abcdefghijklmnopqrstuvwxyz";
}

std::size_t count_sstables(const std::filesystem::path& dir) {
    std::size_t count = 0;
    if (!std::filesystem::exists(dir)) {
        return count;
    }

    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".sst") {
            ++count;
        }
    }
    return count;
}

std::uintmax_t directory_size_bytes(const std::filesystem::path& dir) {
    std::uintmax_t total = 0;
    if (!std::filesystem::exists(dir)) {
        return total;
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
        if (entry.is_regular_file()) {
            total += entry.file_size();
        }
    }
    return total;
}

ReadResult measure_reads(flashlsm::KVStore& store, const std::vector<std::string>& queries) {
    std::size_t found_count = 0;
    std::size_t value_bytes = 0;

    const auto start = Clock::now();
    for (const auto& key : queries) {
        const auto value = store.get(key);
        if (value.has_value()) {
            ++found_count;
            value_bytes += value->size();
        }
    }
    const auto end = Clock::now();

    const auto elapsed_us =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    return ReadResult{
        static_cast<double>(elapsed_us) / static_cast<double>(queries.size()),
        found_count,
        value_bytes,
    };
}

std::vector<std::string> make_queries(std::size_t key_count, std::size_t query_count) {
    std::mt19937_64 rng(42);
    std::uniform_int_distribution<std::size_t> dist(0, key_count - 1);

    std::vector<std::string> queries;
    queries.reserve(query_count);
    for (std::size_t i = 0; i < query_count; ++i) {
        queries.push_back(make_key(dist(rng)));
    }
    return queries;
}

std::vector<std::string> make_missing_queries(std::size_t query_count) {
    std::vector<std::string> queries;
    queries.reserve(query_count);
    for (std::size_t i = 0; i < query_count; ++i) {
        queries.push_back("missing-key-" + std::to_string(i));
    }
    return queries;
}

void run_write_benchmark(const std::filesystem::path& dir) {
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);

    constexpr std::size_t record_count = 1000;
    flashlsm::KVStore store(dir, 1024 * 1024);

    const auto start = Clock::now();
    for (std::size_t i = 0; i < record_count; ++i) {
        store.put(make_key(i), make_value(i));
    }
    store.flush();
    const auto end = Clock::now();

    const auto elapsed_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    const double seconds = static_cast<double>(std::max<std::int64_t>(elapsed_ms, 1)) / 1000.0;
    const double writes_per_second = static_cast<double>(record_count) / seconds;

    std::cout << "Sequential write benchmark\n";
    std::cout << "  records: " << record_count << '\n';
    std::cout << "  elapsed_ms: " << elapsed_ms << '\n';
    std::cout << "  writes_per_second: " << std::fixed << std::setprecision(2)
              << writes_per_second << "\n\n";
}

void run_compaction_read_benchmark(const std::filesystem::path& dir) {
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);

    constexpr std::size_t batch_count = 6;
    constexpr std::size_t records_per_batch = 150;
    constexpr std::size_t key_count = batch_count * records_per_batch;
    constexpr std::size_t query_count = 800;

    flashlsm::KVStore store(dir, 1024 * 1024);

    for (std::size_t batch = 0; batch < batch_count; ++batch) {
        const std::size_t begin = batch * records_per_batch;
        const std::size_t end = begin + records_per_batch;
        for (std::size_t i = begin; i < end; ++i) {
            store.put(make_key(i), make_value(i));
        }
        store.flush();
    }

    const auto queries = make_queries(key_count, query_count);
    const auto missing_queries = make_missing_queries(query_count);

    const auto before_sstables = count_sstables(dir);
    const auto before_bytes = directory_size_bytes(dir);
    const auto before_reads = measure_reads(store, queries);
    const auto before_missing_reads = measure_reads(store, missing_queries);

    const auto compaction_start = Clock::now();
    while (count_sstables(dir) > 2) {
        const auto before_count = count_sstables(dir);
        store.compact();
        if (count_sstables(dir) >= before_count) {
            break;
        }
    }
    const auto compaction_end = Clock::now();

    const auto after_sstables = count_sstables(dir);
    const auto after_bytes = directory_size_bytes(dir);
    const auto after_reads = measure_reads(store, queries);
    const auto after_missing_reads = measure_reads(store, missing_queries);
    const auto compaction_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(compaction_end - compaction_start)
            .count();

    if (before_reads.found_count != query_count || after_reads.found_count != query_count) {
        throw std::runtime_error("benchmark read verification failed");
    }
    if (before_missing_reads.found_count != 0 || after_missing_reads.found_count != 0) {
        throw std::runtime_error("benchmark missing-read verification failed");
    }

    std::cout << "Random read before/after compaction\n";
    std::cout << "  keys: " << key_count << '\n';
    std::cout << "  queries: " << query_count << '\n';
    std::cout << "  sstables_before: " << before_sstables << '\n';
    std::cout << "  sstables_after: " << after_sstables << '\n';
    std::cout << "  disk_bytes_before: " << before_bytes << '\n';
    std::cout << "  disk_bytes_after: " << after_bytes << '\n';
    std::cout << "  compaction_ms: " << compaction_ms << '\n';
    std::cout << "  avg_read_us_before: " << std::fixed << std::setprecision(2)
              << before_reads.average_microseconds << '\n';
    std::cout << "  avg_read_us_after: " << std::fixed << std::setprecision(2)
              << after_reads.average_microseconds << '\n';
    std::cout << "  avg_missing_read_us_before: " << std::fixed << std::setprecision(2)
              << before_missing_reads.average_microseconds << '\n';
    std::cout << "  avg_missing_read_us_after: " << std::fixed << std::setprecision(2)
              << after_missing_reads.average_microseconds << '\n';
    std::cout << "  verification_bytes_before: " << before_reads.value_bytes << '\n';
    std::cout << "  verification_bytes_after: " << after_reads.value_bytes << '\n';
}

}  // namespace

int main() {
    try {
        const std::filesystem::path base_dir = "tmp_benchmark_data";

        run_write_benchmark(base_dir / "writes");
        run_compaction_read_benchmark(base_dir / "reads");

        std::cout << "\nBenchmark complete.\n";
    } catch (const std::exception& ex) {
        std::cerr << "Benchmark failed: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
