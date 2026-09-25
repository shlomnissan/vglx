cmake_minimum_required(VERSION 3.20)

project(vglx VERSION 0.3.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINE_HIDDEN ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(VGLX_LOG_LEVEL "" CACHE STRING "VGLX log level (0=Error, 1=Warning, 2=Info, 3=Debug). Empty follows the build configuration: 3 for Debug, 1 otherwise")

option(VGLX_BUILD_DOCS "Build API documentation using Doxygen" OFF)
option(VGLX_BUILD_EXAMPLES "Build example application" ON)
option(VGLX_BUILD_IMGUI "Build and integrate ImGui from the vendored source" ON)
option(VGLX_BUILD_TESTS "Build unit tests and test infrastructure using GTest" ON)
option(VGLX_BUILD_BENCHMARKS "Build microbenchmarks using Google Benchmark" OFF)
option(VGLX_EXPERIMENTAL_VULKAN "Build the experimental Vulkan renderer instead of OpenGL" OFF)

if (VGLX_EXPERIMENTAL_VULKAN AND VGLX_BUILD_IMGUI)
    message(WARNING "ImGui is not supported by the experimental Vulkan renderer, disabling VGLX_BUILD_IMGUI")
    set(VGLX_BUILD_IMGUI OFF)
endif()

include(GNUInstallDirs)

add_subdirectory("vendor")
add_subdirectory("src")

if (VGLX_BUILD_TESTS)
    include(CTest)
    add_subdirectory("tests")
endif()

if (VGLX_BUILD_BENCHMARKS)
    add_subdirectory("benchmark")
endif()

if (VGLX_BUILD_EXAMPLES)
    add_subdirectory("examples")
endif()

if (VGLX_BUILD_DOCS)
    include(Doxygen)
endif()
