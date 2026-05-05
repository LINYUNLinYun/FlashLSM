cmake_minimum_required(VERSION 3.16)

project(FlashLSM VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(${PROJECT_NAME}
    src/compaction.cpp
    src/kv_store.cpp
    src/memtable.cpp
    src/record.cpp
    src/sstable.cpp
    src/write_ahead_log.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(${PROJECT_NAME}_demo
    src/main.cpp
)

target_link_libraries(${PROJECT_NAME}_demo
    PRIVATE
        ${PROJECT_NAME}
)

add_executable(${PROJECT_NAME}_benchmark
    src/benchmark.cpp
)

target_link_libraries(${PROJECT_NAME}_benchmark
    PRIVATE
        ${PROJECT_NAME}
)

# 编译成静态库 以便测试使用
add_library(testkit STATIC
    testkit/testkit.c
)

target_include_directories(testkit
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/testkit
)

add_executable(${PROJECT_NAME}_tests
    tests/flashlsm_tests.cpp
)

set_target_properties(${PROJECT_NAME}_tests PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS ON
)

target_link_libraries(${PROJECT_NAME}_tests
    PRIVATE
        ${PROJECT_NAME}
        testkit
)

enable_testing()

add_test(
    NAME ${PROJECT_NAME}_tests
    COMMAND ${CMAKE_COMMAND} -E env TK_RUN=1 $<TARGET_FILE:${PROJECT_NAME}_tests>
)
