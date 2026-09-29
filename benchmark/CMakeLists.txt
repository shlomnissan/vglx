find_package(benchmark QUIET)

if (NOT benchmark_FOUND)
    include(FetchContent)
    message(STATUS "📦 Google Benchmark not found, fetching via FetchContent...")
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.9.1
    )
    FetchContent_MakeAvailable(googlebenchmark)
endif()

file(GLOB BENCHMARK_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/*.cpp
)

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_LIST_DIR})

add_executable(run_benchmarks ${BENCHMARK_SOURCES})

target_link_libraries(run_benchmarks PRIVATE benchmark::benchmark_main vglx)
