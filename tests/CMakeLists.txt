# Tests CMakeLists.txt

include(FetchContent)

# Fetch Google Test
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# Fetch Google Benchmark
FetchContent_Declare(
    googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.9.1
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googlebenchmark)

# Enable CTest
include(GoogleTest)

# Test executable
add_executable(firebird_odbc_tests
    test_main.cpp
    test_null_handles.cpp
    test_descriptor.cpp
    test_multi_statement.cpp
    test_data_types.cpp
    test_blob.cpp
    test_savepoint.cpp
    test_escape_sequences.cpp
    test_server_version.cpp
    test_conn_settings.cpp
    test_scrollable_cursor.cpp
    test_connect_options.cpp
    test_errors.cpp
    test_result_conversions.cpp
    test_param_conversions.cpp
    test_prepare.cpp
    test_cursor_commit.cpp
    test_data_at_execution.cpp
    test_array_binding.cpp
    test_odbc38_compliance.cpp
    test_guid_and_binary.cpp
    test_stmthandles.cpp
    test_catalogfunctions.cpp
    test_cursors.cpp
    test_descrec.cpp
    test_bindcol.cpp
    test_wchar.cpp
    test_cursor_name.cpp
    test_odbc_string.cpp
    # Utf16Convert.cpp is compiled into OdbcFb.dll but its symbols are not exported.
    # We include it here so that OdbcString unit tests can link without requiring
    # the full driver DLL at test time.
    ${CMAKE_SOURCE_DIR}/src/Utf16Convert.cpp
)

# Link with Google Test and the ODBC library
target_link_libraries(firebird_odbc_tests
    PRIVATE
        gtest
        gtest_main
        OdbcFb
        IscDbc
)

# Include directories
target_include_directories(firebird_odbc_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/IscDbc
)

# Platform-specific settings
if(WIN32)
    target_link_libraries(firebird_odbc_tests PRIVATE
        odbc32
        odbccp32
    )
else()
    target_link_libraries(firebird_odbc_tests PRIVATE
        ${ODBC_LIBRARIES}
    )
endif()

# Discover individual Google Test cases so CTest runs each one separately.
# Use PRE_TEST discovery mode so the test binary is only executed at ctest time
# (not at build time), avoiding DLL-not-found errors during the build step.
gtest_discover_tests(firebird_odbc_tests
    DISCOVERY_TIMEOUT 30
    DISCOVERY_MODE PRE_TEST
)

# Copy the driver DLL next to the test executable so LoadLibrary finds it
add_custom_command(TARGET firebird_odbc_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:OdbcFb>
        $<TARGET_FILE_DIR:firebird_odbc_tests>
    COMMENT "Copying driver DLL next to test executable"
)

# Copy gtest/gmock DLLs next to the test executable
add_custom_command(TARGET firebird_odbc_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:gtest>
        $<TARGET_FILE_DIR:firebird_odbc_tests>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:gtest_main>
        $<TARGET_FILE_DIR:firebird_odbc_tests>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:gmock>
        $<TARGET_FILE_DIR:firebird_odbc_tests>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:gmock_main>
        $<TARGET_FILE_DIR:firebird_odbc_tests>
    COMMENT "Copying gtest/gmock DLLs next to test executable"
)

# --------------------------------------------------------------------------
# Benchmark executable (Google Benchmark)
# --------------------------------------------------------------------------
add_executable(firebird_odbc_bench bench_fetch.cpp)

target_link_libraries(firebird_odbc_bench PRIVATE benchmark::benchmark OdbcFb IscDbc)

target_include_directories(firebird_odbc_bench PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/IscDbc
)

if(WIN32)
    target_link_libraries(firebird_odbc_bench PRIVATE odbc32 odbccp32)
else()
    target_link_libraries(firebird_odbc_bench PRIVATE ${ODBC_LIBRARIES})
endif()

# Copy DLLs next to the benchmark executable
add_custom_command(TARGET firebird_odbc_bench POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:OdbcFb>
        $<TARGET_FILE_DIR:firebird_odbc_bench>
    COMMENT "Copying driver DLL next to benchmark executable"
)
if(TARGET benchmark)
    add_custom_command(TARGET firebird_odbc_bench POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:benchmark>
            $<TARGET_FILE_DIR:firebird_odbc_bench>
        COMMENT "Copying benchmark DLL next to benchmark executable"
    )
endif()
