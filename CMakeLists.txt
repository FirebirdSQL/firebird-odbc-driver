cmake_minimum_required(VERSION 3.20)

# Auto-bootstrap vcpkg submodule if present but not yet bootstrapped
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
    if(CMAKE_HOST_WIN32)
        set(_vcpkg_exe "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/vcpkg.exe")
        set(_vcpkg_bootstrap "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/bootstrap-vcpkg.bat")
    else()
        set(_vcpkg_exe "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/vcpkg")
        set(_vcpkg_bootstrap "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/bootstrap-vcpkg.sh")
    endif()
    if(NOT EXISTS "${_vcpkg_exe}")
        message(STATUS "Bootstrapping vcpkg submodule...")
        execute_process(
            COMMAND "${_vcpkg_bootstrap}" -disableMetrics
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg"
            RESULT_VARIABLE _vcpkg_bootstrap_result
        )
        if(NOT _vcpkg_bootstrap_result EQUAL 0)
            message(FATAL_ERROR "vcpkg bootstrap failed (exit code ${_vcpkg_bootstrap_result})")
        endif()
        message(STATUS "vcpkg bootstrap complete")
    endif()
endif()

# Auto-detect vcpkg toolchain: submodule first, then environment
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
        set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
    elseif(DEFINED ENV{VCPKG_ROOT})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
    elseif(DEFINED ENV{VCPKG_INSTALLATION_ROOT})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_INSTALLATION_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
    endif()
endif()

project(firebird-odbc-driver LANGUAGES CXX C)

# Extract version from git tags
include(cmake/GetVersionFromGit.cmake)

# Set project version from git
set(PROJECT_VERSION "${ODBC_VERSION_MAJOR}.${ODBC_VERSION_MINOR}.${ODBC_VERSION_PATCH}.${ODBC_VERSION_TWEAK}")
set(PROJECT_VERSION_MAJOR ${ODBC_VERSION_MAJOR})
set(PROJECT_VERSION_MINOR ${ODBC_VERSION_MINOR})
set(PROJECT_VERSION_PATCH ${ODBC_VERSION_PATCH})
set(PROJECT_VERSION_TWEAK ${ODBC_VERSION_TWEAK})

# Generate Version.h from template
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/Version.h
    @ONLY
)

# Set C++ standard (C++20 required for fb-cpp)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Options
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(BUILD_TESTING "Build tests" ON)
option(ODBC_PERF_COUNTERS "Enable performance counter instrumentation" OFF)

if(ODBC_PERF_COUNTERS)
    add_definitions(-DODBC_PERF_COUNTERS)
endif()

# Platform-specific settings
if(WIN32)
    add_definitions(-DWIN32 -D_WINDOWS -D_USRDLL)
    if(MSVC)
        add_definitions(-D_CRT_SECURE_NO_WARNINGS)
        # Use static runtime for static builds
        if(NOT BUILD_SHARED_LIBS)
            set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        endif()
    endif()
else()
    # Linux/Unix
    add_definitions(-DUNIX)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# Enable Link-Time Optimization (LTO) for Release builds
# LTO allows cross-TU inlining (e.g., OdbcConvert::conv* called from OdbcStatement)
# and dead code elimination across the OdbcFb.dll → IscDbc.lib boundary.
include(CheckIPOSupported)
check_ipo_supported(RESULT _ipo_supported OUTPUT _ipo_output)
if(_ipo_supported)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
endif()

# 10.7.5: Architecture-specific instruction scheduling for Release builds
if(MSVC)
    # /favor:AMD64 — optimize scheduling for AMD64 (x86-64) processors
    add_compile_options($<$<CONFIG:Release>:/favor:AMD64>)
    add_compile_options($<$<CONFIG:RelWithDebInfo>:/favor:AMD64>)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # -march=native — enable all instruction sets supported by the build host
    add_compile_options($<$<CONFIG:Release>:-march=native>)
    add_compile_options($<$<CONFIG:RelWithDebInfo>:-march=native>)
endif()

# Find required packages
if(WIN32)
    # On Windows, ODBC is typically part of the system
    set(ODBC_FOUND TRUE)
else()
    # On Unix-like systems, find ODBC (unixODBC)
    find_package(ODBC REQUIRED)
    if(ODBC_FOUND)
        include_directories(${ODBC_INCLUDE_DIRS})
    endif()
endif()

# Find fb-cpp (modern C++ wrapper for Firebird) — provides Firebird headers transitively
find_package(fb-cpp CONFIG REQUIRED)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_BINARY_DIR}/generated
)

# Add subdirectories
add_subdirectory(src/core)

# Windows resource file for version info
if(WIN32)
    set(ODBCJDBC_RC src/OdbcJdbc.rc)
endif()

# Main ODBC driver library sources
set(ODBCJDBC_SOURCES
    src/ConnectDialog.cpp
    src/DescRecord.cpp
    src/Main.cpp
    src/MainUnicode.cpp
    src/MbsAndWcs.cpp
    src/OdbcConnection.cpp
    src/OdbcConvert.cpp
    src/OdbcDesc.cpp
    src/OdbcEnv.cpp
    src/OdbcError.cpp
    src/OdbcObject.cpp
    src/OdbcStatement.cpp
    src/SafeEnvThread.cpp
    src/Utf16Convert.cpp
    src/OdbcPerfCounters.cpp
)

set(ODBCJDBC_HEADERS
    src/ConnectDialog.h
    src/OdbcPerfCounters.h
    src/DescRecord.h
    src/InfoItems.h
    src/Main.h
    src/OdbcConnection.h
    src/OdbcConvert.h
    src/OdbcDesc.h
    src/OdbcEnv.h
    src/OdbcError.h
    src/OdbcJdbc.h
    src/OdbcObject.h
    src/OdbcStatement.h
    src/SafeEnvThread.h
    src/SecurityPassword.h
    src/SetupAttributes.h
    src/TemplateConvert.h
    src/Utf16Convert.h
    src/OdbcString.h
    src/OdbcSqlState.h
    src/OdbcUserEvents.h
    src/resource.h
)

# Create the main ODBC driver library
add_library(OdbcFb ${ODBCJDBC_SOURCES} ${ODBCJDBC_HEADERS} ${ODBCJDBC_RC})

# Set library output name
if(WIN32)
    set_target_properties(OdbcFb PROPERTIES OUTPUT_NAME "FirebirdODBC")
else()
    set_target_properties(OdbcFb PROPERTIES OUTPUT_NAME "OdbcFb")
endif()

# Link with IscDbc library
target_link_libraries(OdbcFb PRIVATE IscDbc)

# Platform-specific linking
if(WIN32)
    target_link_libraries(OdbcFb PRIVATE 
        odbccp32 
        legacy_stdio_definitions
        Version
    )
    
    # Don't link against odbc32.dll - the driver provides its own
    # ODBC entry points and should not import them from the Driver Manager
    
    # Set the .def file for exports
    set_target_properties(OdbcFb PROPERTIES LINK_FLAGS "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/src/OdbcJdbc.def")
else()
    target_link_libraries(OdbcFb PRIVATE 
        ${ODBC_LIBRARIES}
        odbcinst
        dl
        pthread
    )
endif()

# Testing
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation rules
install(TARGETS OdbcFb
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Install headers (optional, for development)
install(FILES ${ODBCJDBC_HEADERS}
    DESTINATION include/firebird-odbc
)
