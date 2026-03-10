# Custom triplet: force ICU to build as dynamic library.
# The Firebird client loads ICU at runtime for time-zone handling,
# so ICU must be a shared library even when the rest is static.

set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

if(PORT MATCHES "^(icu|firebird)$")
    set(VCPKG_LIBRARY_LINKAGE dynamic)
endif()
