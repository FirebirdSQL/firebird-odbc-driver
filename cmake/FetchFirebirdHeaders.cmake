# FetchFirebirdHeaders.cmake
#
# Downloads the Firebird public C/C++ API headers from the official
# FirebirdSQL/firebird GitHub repository at a pinned release tag.
#
# After including this module the variable FIREBIRD_INCLUDE_DIR points
# to the directory that contains both the legacy compatibility stubs
# (ibase.h, iberror.h) and the firebird/ sub-tree with the modern API
# (Interface.h, IdlFbInterfaces.h, Message.h, impl/, …).
#
# Usage:
#   include(cmake/FetchFirebirdHeaders.cmake)
#   target_include_directories(MyTarget PRIVATE ${FIREBIRD_INCLUDE_DIR})

include(FetchContent)

# Pin to Firebird 5.0.2 – bump when upgrading the target engine version.
set(FIREBIRD_VERSION "5.0.2" CACHE STRING "Firebird version to fetch headers for")
set(FIREBIRD_TAG     "v${FIREBIRD_VERSION}")

FetchContent_Declare(
    firebird_headers
    URL      "https://github.com/FirebirdSQL/firebird/archive/refs/tags/${FIREBIRD_TAG}.zip"
    # The full source archive is ~40 MB compressed.  CMake caches the
    # download in the build tree so it is only fetched once.
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    # Point SOURCE_SUBDIR to a path without a CMakeLists.txt so that
    # FetchContent_MakeAvailable downloads the archive but does NOT try
    # to configure the full Firebird build system as a sub-project.
    SOURCE_SUBDIR src/include
)

FetchContent_MakeAvailable(firebird_headers)

# The public headers live under src/include/ inside the archive.
# That directory contains the legacy stubs (ibase.h → firebird/ibase.h)
# *and* the firebird/ sub-tree.
set(FIREBIRD_INCLUDE_DIR "${firebird_headers_SOURCE_DIR}/src/include"
    CACHE PATH "Firebird public header include directory" FORCE)

message(STATUS "Firebird headers (${FIREBIRD_TAG}): ${FIREBIRD_INCLUDE_DIR}")
