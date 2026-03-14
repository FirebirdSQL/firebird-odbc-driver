cmake_minimum_required(VERSION 3.15)

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

# Platform-specific settings
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
    add_definitions(-DUNIX)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# Find required packages
if(WIN32)
    # On Windows, ODBC is part of the system SDK
    set(ODBC_FOUND TRUE)
else()
    # On Unix-like systems, find ODBC (unixODBC)
    find_package(ODBC REQUIRED)
    if(ODBC_FOUND)
        include_directories(${ODBC_INCLUDE_DIRS})
    endif()
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

# Add IscDbc static library
add_subdirectory(IscDbc)

# Windows resource file for version info
if(WIN32)
    set(ODBCJDBC_RC OdbcJdbc.rc)
endif()

# Main ODBC driver library sources (from makefile.sources ODBCJDBCSRC)
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
    ResourceManagerSink.cpp
    SafeEnvThread.cpp
    TransactionResourceAsync.cpp
)

# OdbcJdbcSetup sources — built into the same DLL (from makefile.sources ODBCJDBCSETUPSRC)
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

# Linux-only setup source
if(NOT WIN32)
    list(APPEND ODBCJDBCSETUP_SOURCES OdbcInstGetProp.cpp)
endif()

# Create the main ODBC driver library
add_library(OdbcFb
    ${ODBCJDBC_SOURCES}
    ${ODBCJDBCSETUP_SOURCES}
    ${ODBCJDBC_RC}
)

# Set library output name
if(WIN32)
    set_target_properties(OdbcFb PROPERTIES OUTPUT_NAME "FirebirdODBC")
else()
    set_target_properties(OdbcFb PROPERTIES OUTPUT_NAME "OdbcFb")
endif()

# Link with IscDbc library
target_link_libraries(OdbcFb PRIVATE IscDbc)

# Platform-specific linking (matching the .vcxproj AdditionalDependencies)
if(WIN32)
    target_link_libraries(OdbcFb PRIVATE
        odbccp32
        version
        wsock32
        comctl32
        legacy_stdio_definitions
        legacy_stdio_wide_specifiers
    )

    # Set the .def file for exports (matching the .vcxproj ModuleDefinitionFile)
    # Disable automatic manifest generation — the .rc file already embeds one.
    # Add explicit exports matching the .vcxproj /EXPORT: flags
    set_target_properties(OdbcFb PROPERTIES
        LINK_FLAGS "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/OdbcJdbc.def /MANIFEST:NO /EXPORT:ConfigDSN /EXPORT:ConfigDriver,PRIVATE /EXPORT:DllRegisterServer,PRIVATE /EXPORT:DllUnregisterServer,PRIVATE /EXPORT:DllInstall,PRIVATE"
    )
else()
    target_link_libraries(OdbcFb PRIVATE
        ${ODBC_LIBRARIES}
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
