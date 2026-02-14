# Minimum CMake version chosen for modern target features
cmake_minimum_required(VERSION 3.20)

project(HFTToolset VERSION 0.1.0 LANGUAGES CXX)

# Enforce modern C++ and keep flags consistent across consumers
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Static library target
add_library(HFTToolset
    STATIC
    src/library_anchor.cpp
    src/Market/order_book.h src/Market/order_book.cpp
    src/Market/matching_engine.h src/Market/matching_engine.cpp
    src/Market/market_data_publisher.h src/Market/market_data_publisher.cpp
    src/benchmark_p99.hpp
    src/ScopeTimer.hpp
    src/HPRingBuffer.hpp
    src/common/types.h
    src/common/clock.h src/common/clock.cpp
)


add_library(HFT::Toolset ALIAS HFTToolset)

target_include_directories(HFTToolset
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/common>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(HFTToolset PUBLIC cxx_std_23)

