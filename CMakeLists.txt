cmake_minimum_required(VERSION 3.25)

project(vglx VERSION 0.4.0)

if (PROJECT_IS_TOP_LEVEL)
    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

set(VGLX_LOG_LEVEL "" CACHE STRING "VGLX log level (0=Error, 1=Warning, 2=Info, 3=Debug). Empty follows the build configuration: 3 for Debug, 1 otherwise")

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(VGLX_BUILD_DOCS "Build API documentation using Doxygen" OFF)
option(VGLX_BUILD_EXAMPLES "Build example application" ${PROJECT_IS_TOP_LEVEL})
option(VGLX_BUILD_IMGUI "Build and integrate ImGui from the vendored source" ${PROJECT_IS_TOP_LEVEL})
option(VGLX_BUILD_TESTS "Build unit tests and test infrastructure using GTest" ${PROJECT_IS_TOP_LEVEL})
option(VGLX_BUILD_BENCHMARKS "Build microbenchmarks using Google Benchmark" OFF)
option(VGLX_EXPERIMENTAL_VULKAN "Build the experimental Vulkan renderer instead of OpenGL" OFF)
option(VGLX_STATIC_MOLTENVK "Link MoltenVK into the Vulkan backend on Apple platforms" ${APPLE})
option(VGLX_INSTALL "Generate install rules and the CMake package" ${PROJECT_IS_TOP_LEVEL})

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
    include("${PROJECT_SOURCE_DIR}/cmake/Doxygen.cmake")
endif()
