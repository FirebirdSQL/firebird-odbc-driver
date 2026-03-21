cmake_minimum_required(VERSION 3.15)

project(firebird-odbc-driver LANGUAGES CXX C)

# ---------------------------------------------------------------------------
# Default build type (single-config generators only, e.g. Unix Makefiles/Ninja)
# ---------------------------------------------------------------------------
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

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

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Options
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(BUILD_TESTING "Build tests" ON)

# ---------------------------------------------------------------------------
# CPU architecture detection (from PR #248)
# ---------------------------------------------------------------------------
set(FBODBC_ARCH ${CMAKE_HOST_SYSTEM_PROCESSOR})

if(FBODBC_ARCH STREQUAL "x86" OR FBODBC_ARCH STREQUAL "i686")
    add_definitions(-DFBODBC_ARCH_X86)
    message(STATUS "Architecture: ${FBODBC_ARCH} (x86)")
elseif(FBODBC_ARCH STREQUAL "x86_64" OR FBODBC_ARCH STREQUAL "AMD64")
    add_definitions(-DFBODBC_ARCH_X86_64)
    message(STATUS "Architecture: ${FBODBC_ARCH} (x86_64)")
elseif(FBODBC_ARCH STREQUAL "aarch64" OR FBODBC_ARCH STREQUAL "arm64" OR FBODBC_ARCH STREQUAL "ARM64")
    add_definitions(-DFBODBC_ARCH_ARM64)
    message(STATUS "Architecture: ${FBODBC_ARCH} (ARM64)")
else()
    add_definitions(-DFBODBC_ARCH_GENERIC)
    message(WARNING "Architecture <${FBODBC_ARCH}> conditionally supported")
endif()

# ---------------------------------------------------------------------------
# Platform-specific compiler settings
# ---------------------------------------------------------------------------
if(WIN32)
    add_definitions(-DWIN32 -D_WINDOWS -D_USRDLL)
    if(MSVC)
        add_definitions(-D_CRT_SECURE_NO_WARNINGS)
        # Match the VS 2022 project: static CRT (/MT for Release, /MTd for Debug)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        # Suppress same warnings as the .vcxproj (C4996, C5033)
        add_compile_options(/wd4996 /wd5033)
    endif()
else()
    # Linux/Unix
    add_definitions(-DUNIX -D_REENTRANT -D_PTHREADS -DEXTERNAL)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)

    # Compiler optimization flags (from PR #248 / old makefile.linux)
    add_compile_options(
        "$<$<CONFIG:Debug>:-O0;-g3;-D_DEBUG;-DDEBUG;-DLOGGING;-fexceptions>"
        "$<$<CONFIG:Release>:-O3;-DNDEBUG;-ftree-loop-vectorize>"
        "$<$<CONFIG:RelWithDebInfo>:-O2;-g;-DNDEBUG>"
        "$<$<CONFIG:MinSizeRel>:-Os;-DNDEBUG>"
    )

    # SSE4.1 for x86/x86_64 (from PR #248)
    if(FBODBC_ARCH STREQUAL "x86" OR FBODBC_ARCH STREQUAL "i686"
       OR FBODBC_ARCH STREQUAL "x86_64" OR FBODBC_ARCH STREQUAL "AMD64")
        add_compile_options(-msse4.1)
    endif()
endif()

# ---------------------------------------------------------------------------
# Find required packages
# ---------------------------------------------------------------------------
if(WIN32)
    # On Windows, ODBC is part of the system SDK
    set(ODBC_FOUND TRUE)
else()
    # On Unix-like systems, find ODBC (unixODBC)
    find_package(ODBC REQUIRED)
    add_definitions(-DunixODBC)
endif()

# Fetch Firebird public headers from GitHub (replaces vendored FBClient.Headers)
include(cmake/FetchFirebirdHeaders.cmake)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/IscDbc
    ${CMAKE_CURRENT_BINARY_DIR}/generated
    ${FIREBIRD_INCLUDE_DIR}
)

# ---------------------------------------------------------------------------
# IscDbc — Firebird database connectivity (object library)
# ---------------------------------------------------------------------------
add_subdirectory(IscDbc)

# ---------------------------------------------------------------------------
# Windows resource file for version info
# ---------------------------------------------------------------------------
if(WIN32)
    set(ODBCJDBC_RC OdbcJdbc.rc)
endif()

# ---------------------------------------------------------------------------
# Main ODBC driver library sources (from makefile.sources ODBCJDBCSRC)
# ---------------------------------------------------------------------------
set(ODBCJDBC_SOURCES
    ConnectDialog.cpp
    DescRecord.cpp
    Main.cpp
    MainUnicode.cpp
    MbsAndWcs.cpp
    OdbcConnection.cpp
    OdbcConvert.cpp
    OdbcDateTime.cpp
    OdbcDesc.cpp
    OdbcEnv.cpp
    OdbcError.cpp
    OdbcObject.cpp
    OdbcStatement.cpp
    SafeEnvThread.cpp
)

# Windows-only sources (ATL-based DTC support, wrapped in #ifdef _WINDOWS)
if(WIN32)
    list(APPEND ODBCJDBC_SOURCES
        ResourceManagerSink.cpp
        TransactionResourceAsync.cpp
    )
endif()

# ---------------------------------------------------------------------------
# OdbcJdbcSetup sources — platform-specific (from makefile.sources)
# ---------------------------------------------------------------------------
if(WIN32)
    # Windows: full GUI setup (ODBCJDBCSETUPSRC)
    set(ODBCJDBCSETUP_SOURCES
        OdbcJdbcSetup/CommonUtil.cpp
        OdbcJdbcSetup/DsnDialog.cpp
        OdbcJdbcSetup/OdbcJdbcSetup.cpp
        OdbcJdbcSetup/ServiceClient.cpp
        OdbcJdbcSetup/ServiceTabBackup.cpp
        OdbcJdbcSetup/ServiceTabChild.cpp
        OdbcJdbcSetup/ServiceTabCtrl.cpp
        OdbcJdbcSetup/ServiceTabRepair.cpp
        OdbcJdbcSetup/ServiceTabRestore.cpp
        OdbcJdbcSetup/ServiceTabStatistics.cpp
        OdbcJdbcSetup/ServiceTabUsers.cpp
        OdbcJdbcSetup/Setup.cpp
        OdbcJdbcSetup/UserDialog.cpp
        OdbcJdbcSetup/UsersTabChild.cpp
        OdbcJdbcSetup/UsersTabMemberShips.cpp
        OdbcJdbcSetup/UsersTabRoles.cpp
        OdbcJdbcSetup/UsersTabUsers.cpp
    )
else()
    # Linux: only OdbcInstGetProp (ODBCJDBCSETUPSRC_LINUX)
    set(ODBCJDBCSETUP_SOURCES
        OdbcInstGetProp.cpp
    )
endif()

# ---------------------------------------------------------------------------
# Create the main ODBC driver library
# ---------------------------------------------------------------------------
add_library(OdbcFb
    ${ODBCJDBC_SOURCES}
    ${ODBCJDBCSETUP_SOURCES}
    ${ODBCJDBC_RC}
    $<TARGET_OBJECTS:IscDbc>
)

# Set library output name
if(WIN32)
    set_target_properties(OdbcFb PROPERTIES OUTPUT_NAME "FirebirdODBC")
else()
    set_target_properties(OdbcFb PROPERTIES OUTPUT_NAME "OdbcFb")
endif()

# ---------------------------------------------------------------------------
# Linking
# ---------------------------------------------------------------------------
if(WIN32)
    # Match the .vcxproj AdditionalDependencies
    target_link_libraries(OdbcFb PRIVATE
        odbccp32
        version
        wsock32
        comctl32
        legacy_stdio_definitions
        legacy_stdio_wide_specifiers
    )

    # .def file for exports + disable duplicate manifest + extra exports
    set_target_properties(OdbcFb PROPERTIES
        LINK_FLAGS "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/OdbcJdbc.def /MANIFEST:NO /EXPORT:ConfigDSN /EXPORT:ConfigDriver,PRIVATE /EXPORT:DllRegisterServer,PRIVATE /EXPORT:DllUnregisterServer,PRIVATE /EXPORT:DllInstall,PRIVATE"
    )
else()
    target_link_libraries(OdbcFb PRIVATE
        ODBC::ODBC
        odbcinst
        dl
        pthread
    )
endif()

# Debug-specific definitions (matching .vcxproj: DEBUG;LOGGING for Debug configs)
target_compile_definitions(OdbcFb PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Debug>:LOGGING>
)

# ---------------------------------------------------------------------------
# Testing
# ---------------------------------------------------------------------------
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

# ---------------------------------------------------------------------------
# Installation
# ---------------------------------------------------------------------------
install(TARGETS OdbcFb
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
