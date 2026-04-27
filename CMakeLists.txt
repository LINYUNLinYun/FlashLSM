cmake_minimum_required(VERSION 3.16)

project(FlashLSM VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(flashlsm
    src/kv_store.cpp
    src/memtable.cpp
    src/record.cpp
    src/sstable.cpp
    src/write_ahead_log.cpp
)

target_include_directories(flashlsm
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
