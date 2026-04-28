#include "flashlsm/kv_store.h"

#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

namespace {

void expect_equal(const std::optional<std::string>& actual,
                  const std::optional<std::string>& expected,
                  const std::string& case_name) {
    if (actual != expected) {
        std::cerr << "[FAILED] " << case_name << '\n';
        std::cerr << "  expected: "
                  << (expected ? *expected : "nullopt")
                  << '\n';
        std::cerr << "  actual  : "
                  << (actual ? *actual : "nullopt")
                  << '\n';
        throw std::runtime_error("test case failed");
    }

    std::cout << "[PASSED] " << case_name << '\n';
}

}  // namespace

int main() {
    const std::filesystem::path data_dir = "tmp_demo_data";

    // 这是演示/测试目录，可以安全清理。
    std::filesystem::remove_all(data_dir);
    std::filesystem::create_directories(data_dir);

    try {
        flashlsm::KVStore store(data_dir, 1024 * 1024);

        store.put("a", "1");
        expect_equal(store.get("a"), std::optional<std::string>("1"), "put 后立即 get");

        store.put("a", "2");
        expect_equal(store.get("a"), std::optional<std::string>("2"), "同 key 覆盖写");

        store.remove("a");
        expect_equal(store.get("a"), std::nullopt, "remove 后读取为空");

        store.put("b", "disk-value");
        store.flush();
        expect_equal(store.get("b"), std::optional<std::string>("disk-value"), "flush 后仍可读取");

        store.remove("b");
        expect_equal(store.get("b"), std::nullopt, "flush 后再删除");

        std::cout << "\nAll demo checks passed.\n";
    } catch (const std::exception& ex) {
        std::cerr << "\nDemo failed: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
