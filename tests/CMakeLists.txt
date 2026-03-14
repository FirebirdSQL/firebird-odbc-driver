# Firebird ODBC Driver — Test Suite
#
# Tests the driver through the standard ODBC API (via the Driver Manager).
#
# This file is included from the root CMakeLists.txt via add_subdirectory().
# It can also be built standalone for quick iteration:
#
#   cmake -B build-tests -S tests -DCMAKE_PREFIX_PATH=<vcpkg-root>/installed/x64-windows
#   cmake --build build-tests --config Release
#   set FIREBIRD_ODBC_CONNECTION=Driver={Firebird ODBC Driver};...
#   ctest --test-dir build-tests --output-on-failure

# When built standalone (no parent project), set up the project and standards.
if(NOT CMAKE_PROJECT_NAME OR CMAKE_PROJECT_NAME STREQUAL "FirebirdOdbcTests")
    cmake_minimum_required(VERSION 3.14)
    project(FirebirdOdbcTests LANGUAGES CXX)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

# ---------------------------------------------------------------------------
# Google Test — prefer FetchContent (integrated build), fall back to find_package
# ---------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
# Prevent GTest from overriding parent project settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
# Build GTest as a static library (the test exe doesn't need a separate DLL)
set(BUILD_SHARED_LIBS_SAVED ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS OFF)
FetchContent_MakeAvailable(googletest)
set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_SAVED})

# ---------------------------------------------------------------------------
# Find ODBC
# ---------------------------------------------------------------------------
find_package(ODBC REQUIRED)

# ---------------------------------------------------------------------------
# Enable CTest
# ---------------------------------------------------------------------------
include(GoogleTest)
if(NOT CMAKE_PROJECT_NAME OR CMAKE_PROJECT_NAME STREQUAL "FirebirdOdbcTests")
    enable_testing()
endif()

# ---------------------------------------------------------------------------
# Test executable
# ---------------------------------------------------------------------------
add_executable(firebird_odbc_tests
    test_main.cpp

    # Category A — tests expected to pass on vanilla master
    test_data_types.cpp
    test_result_conversions.cpp
    test_param_conversions.cpp
    test_prepare.cpp
    test_cursors.cpp
    test_cursor_commit.cpp
    test_cursor_name.cpp
    test_data_at_execution.cpp
    test_array_binding.cpp
    test_bindcol.cpp
    test_descrec.cpp
    test_blob.cpp
    test_multi_statement.cpp
    test_stmthandles.cpp
    test_wchar.cpp
    test_escape_sequences.cpp

    # Category B — mixed pass/skip
    test_descriptor.cpp
    test_connect_options.cpp
    test_errors.cpp
    test_catalogfunctions.cpp
    test_server_version.cpp
    test_scrollable_cursor.cpp

    # Category C — all tests SKIP'd (features not yet on upstream master)
    test_null_handles.cpp
    test_savepoint.cpp
    test_conn_settings.cpp
    test_odbc38_compliance.cpp
    test_guid_and_binary.cpp
    test_odbc_string.cpp
    # NOTE: test_phase7_crusher_fixes.cpp and test_phase11_typeinfo_timeout_pool.cpp
    # are NOT included here — their tests are already present (with GTEST_SKIP markers)
    # in the Category B files (test_descriptor.cpp, test_connect_options.cpp,
    # test_errors.cpp, test_catalogfunctions.cpp). The phase-specific files exist as
    # documentation of which tests belong to which improvement phase.
)

# ---------------------------------------------------------------------------
# Link against Google Test and the ODBC Driver Manager
# ---------------------------------------------------------------------------
target_link_libraries(firebird_odbc_tests PRIVATE gtest ODBC::ODBC)

# ---------------------------------------------------------------------------
# Discover tests for CTest
# ---------------------------------------------------------------------------
gtest_discover_tests(firebird_odbc_tests
    DISCOVERY_TIMEOUT 30
    DISCOVERY_MODE PRE_TEST
)
