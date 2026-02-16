# Firebird ODBC Driver — Test Suite
#
# Standalone CMake project for the Google Test-based test suite.
# Tests the driver through the standard ODBC API (via the Driver Manager).
#
# Prerequisites:
#   - The Firebird ODBC driver must be pre-built and registered as an ODBC
#     data source (or referenced via a Driver= connection string).
#   - A Firebird database must be running and accessible.
#   - The FIREBIRD_ODBC_CONNECTION environment variable must be set.
#
# Build and run:
#   cmake -B build-tests -S tests
#   cmake --build build-tests --config Release
#   set FIREBIRD_ODBC_CONNECTION=Driver={Firebird/InterBase(r) driver};Dbname=localhost:C:\path\to\test.fdb;Uid=SYSDBA;Pwd=masterkey;
#   ctest --test-dir build-tests --output-on-failure

cmake_minimum_required(VERSION 3.14)
project(FirebirdOdbcTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Fetch Google Test
# ---------------------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# ---------------------------------------------------------------------------
# Enable CTest
# ---------------------------------------------------------------------------
include(GoogleTest)
enable_testing()

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
target_link_libraries(firebird_odbc_tests PRIVATE gtest)

if(WIN32)
    target_link_libraries(firebird_odbc_tests PRIVATE odbc32 odbccp32)
else()
    find_package(PkgConfig)
    if(PkgConfig_FOUND)
        pkg_check_modules(ODBC IMPORTED_TARGET odbc)
        if(ODBC_FOUND)
            target_link_libraries(firebird_odbc_tests PRIVATE PkgConfig::ODBC)
        else()
            target_link_libraries(firebird_odbc_tests PRIVATE odbc)
        endif()
    else()
        target_link_libraries(firebird_odbc_tests PRIVATE odbc)
    endif()
endif()

# ---------------------------------------------------------------------------
# Discover tests for CTest
# ---------------------------------------------------------------------------
gtest_discover_tests(firebird_odbc_tests
    DISCOVERY_TIMEOUT 30
    DISCOVERY_MODE PRE_TEST
)
