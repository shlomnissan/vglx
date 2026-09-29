find_package(GTest QUIET)

if (NOT GTest_FOUND)
    include(FetchContent)
    message(STATUS "📦 GTest not found, fetching via FetchContent...")
    set(INSTALL_GMOCK ON CACHE BOOL "" FORCE)
    set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    FetchContent_MakeAvailable(googletest)
endif()

file(GLOB TEST_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/**/*.cpp
)

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_LIST_DIR})

add_executable(run_unit_tests ${TEST_SOURCES} test_helpers.hpp)

target_compile_definitions(run_unit_tests PRIVATE
    ASSETS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
)

target_link_libraries(run_unit_tests PRIVATE GTest::gtest_main GTest::gmock vglx)

add_test(vglx_tests run_unit_tests)