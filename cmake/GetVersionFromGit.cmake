# GetVersionFromGit.cmake
# Extracts version information from git tags in v<MAJOR>.<MINOR>.<PATCH> format.
#
# Sets the following variables in the parent scope:
#   ODBC_VERSION_MAJOR   - Major version number
#   ODBC_VERSION_MINOR   - Minor version number
#   ODBC_VERSION_PATCH   - Patch version number
#   ODBC_VERSION_TWEAK   - Build number (0 for tagged releases, commits-since-tag + 1 for dev builds)
#   ODBC_VERSION_STRING  - Full version string "MAJOR.MINOR.PATCH.TWEAK"
#   ODBC_VERSION_IS_RELEASE - TRUE if building from an exact tag (no extra commits)

find_package(Git QUIET)

if(NOT GIT_FOUND)
    message(WARNING "Git not found, using fallback version 0.0.0.0")
    set(ODBC_VERSION_MAJOR 0)
    set(ODBC_VERSION_MINOR 0)
    set(ODBC_VERSION_PATCH 0)
    set(ODBC_VERSION_TWEAK 0)
    set(ODBC_VERSION_STRING "0.0.0.0")
    set(ODBC_VERSION_IS_RELEASE FALSE)
    return()
endif()

# Try to find the latest semver tag matching v<MAJOR>.<MINOR>.<PATCH>
# We use --match to only consider tags in vX.Y.Z format (not rc, temp, etc.)
execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --tags --match "v[0-9]*.[0-9]*.[0-9]*"
            --exclude "*-*" --long --always
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_DESCRIBE_OUTPUT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_VARIABLE GIT_DESCRIBE_ERROR
    RESULT_VARIABLE GIT_DESCRIBE_RESULT
)

if(NOT GIT_DESCRIBE_RESULT EQUAL 0)
    # No matching tag found at all, try without --exclude
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --match "v[0-9]*.[0-9]*.[0-9]*"
                --long --always
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_DESCRIBE_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_VARIABLE GIT_DESCRIBE_ERROR
        RESULT_VARIABLE GIT_DESCRIBE_RESULT
    )
endif()

if(NOT GIT_DESCRIBE_RESULT EQUAL 0)
    message(WARNING "git describe failed: ${GIT_DESCRIBE_ERROR}")
    message(WARNING "Using fallback version 0.0.0.0")
    set(ODBC_VERSION_MAJOR 0)
    set(ODBC_VERSION_MINOR 0)
    set(ODBC_VERSION_PATCH 0)
    set(ODBC_VERSION_TWEAK 0)
    set(ODBC_VERSION_STRING "0.0.0.0")
    set(ODBC_VERSION_IS_RELEASE FALSE)
    return()
endif()

message(STATUS "Git describe: ${GIT_DESCRIBE_OUTPUT}")

# Parse the output: v<MAJOR>.<MINOR>.<PATCH>-<COMMITS_SINCE_TAG>-g<HASH>
# Example: v3.0.0-0-gabcdef1  (exactly on tag)
# Example: v3.0.0-5-gabcdef1  (5 commits after tag)
if(GIT_DESCRIBE_OUTPUT MATCHES "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)-([0-9]+)-g[0-9a-f]+$")
    set(ODBC_VERSION_MAJOR ${CMAKE_MATCH_1})
    set(ODBC_VERSION_MINOR ${CMAKE_MATCH_2})
    set(ODBC_VERSION_PATCH ${CMAKE_MATCH_3})
    set(_commits_since_tag ${CMAKE_MATCH_4})
else()
    message(WARNING "Could not parse git describe output: ${GIT_DESCRIBE_OUTPUT}")
    message(WARNING "Using fallback version 0.0.0.0")
    set(ODBC_VERSION_MAJOR 0)
    set(ODBC_VERSION_MINOR 0)
    set(ODBC_VERSION_PATCH 0)
    set(ODBC_VERSION_TWEAK 0)
    set(ODBC_VERSION_STRING "0.0.0.0")
    set(ODBC_VERSION_IS_RELEASE FALSE)
    return()
endif()

# Determine the 4th version component (tweak/build number):
#   - Official releases (CI, triggered by a tag push): always 0
#   - Local/dev builds: commits_since_tag + 1 (so that on-tag dev builds = 1, next commit = 2, etc.)
#
# CI is detected via the ODBC_OFFICIAL_RELEASE CMake variable or the CI environment variable.
if(ODBC_OFFICIAL_RELEASE OR DEFINED ENV{CI})
    set(ODBC_VERSION_TWEAK 0)
    set(ODBC_VERSION_IS_RELEASE TRUE)
else()
    if(_commits_since_tag EQUAL 0)
        # Sitting exactly on a tag, but building locally → tweak = 1
        # This distinguishes a local build from the official tagged release
        set(ODBC_VERSION_TWEAK 1)
    else()
        math(EXPR ODBC_VERSION_TWEAK "${_commits_since_tag} + 1")
    endif()
    set(ODBC_VERSION_IS_RELEASE FALSE)
endif()

set(ODBC_VERSION_STRING "${ODBC_VERSION_MAJOR}.${ODBC_VERSION_MINOR}.${ODBC_VERSION_PATCH}.${ODBC_VERSION_TWEAK}")

message(STATUS "Firebird ODBC Driver version: ${ODBC_VERSION_STRING}")
message(STATUS "  Major: ${ODBC_VERSION_MAJOR}")
message(STATUS "  Minor: ${ODBC_VERSION_MINOR}")
message(STATUS "  Patch: ${ODBC_VERSION_PATCH}")
message(STATUS "  Tweak: ${ODBC_VERSION_TWEAK}")
message(STATUS "  Release: ${ODBC_VERSION_IS_RELEASE}")
