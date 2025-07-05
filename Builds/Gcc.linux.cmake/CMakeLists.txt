cmake_minimum_required(VERSION 3.8)
project(firebird_odbc_driver CXX)

if(NOT DEFINED CACHE{CMAKE_BUILD_TYPE})
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Cmake build type")
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

add_compile_options(
    "$<$<CONFIG:DEBUG>:-O0;-g3;-D_DEBUG;-DDEBUG;-DLOGGING;-fexceptions>"
    "$<$<CONFIG:RELEASE>:-O3;-DNDEBUG>"
)

set(FBODBC_ARCH ${CMAKE_HOST_SYSTEM_PROCESSOR})

if(${FBODBC_ARCH} STREQUAL "x86")
    add_definitions(-DFBODBC_ARCH_X86)
    set(FBODBC_ARCH_X86 1 CACHE BOOL "Arch x86")
    message(STATUS "Will compile for ${FBODBC_ARCH} architecture")

elseif(${FBODBC_ARCH} STREQUAL "x86_64")
    add_definitions(-DFBODBC_ARCH_X86_64)
    set(FBODBC_ARCH_X86_64 1 CACHE BOOL "Arch x86_64")
    message(STATUS "Will compile for ${FBODBC_ARCH} architecture")

elseif(${FBODBC_ARCH} STREQUAL "aarch64" OR ${FBODBC_ARCH} STREQUAL "arm64")
    add_definitions(-DFBODBC_ARCH_ARM64)
    set(FBODBC_ARCH_ARM64 1 CACHE BOOL "Arch ARM64")
    message(STATUS "Will compile for ${FBODBC_ARCH} architecture")

else()
    add_definitions(-DFBODBC_ARCH_GENERIC)
    set(FBODBC_ARCH_GENERIC 1 CACHE BOOL "Arch generic")
    message(WARNING "Architecture <${FBODBC_ARCH}> conditionally supported")
endif()

find_package(ODBC REQUIRED)

if(ODBC_FOUND)
    message(STATUS "ODBC found: ${ODBC_INCLUDE_DIRS} ${ODBC_LIBRARIES}")
endif(ODBC_FOUND)

include_directories(${ODBC_INCLUDE_DIRS})

set(FBODBC_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../..")

include_directories("${FBODBC_SRC_DIR}/Headers")
include_directories("${FBODBC_SRC_DIR}/FBClient.Headers")

file(READ "../makefile.sources" ConfigContents)
string(REPLACE "\\\n" "" ConfigContents "${ConfigContents}")
string(REPLACE "\n" ";" ConfigContents "${ConfigContents}")

foreach(NameAndValue ${ConfigContents})
  # Strip leading spaces
  string(REGEX REPLACE "^[ ]+" "" NameAndValue ${NameAndValue})
  # Find variable name
  string(REGEX MATCH "^[^=]+" Name ${NameAndValue})
  # Find the value
  string(REPLACE "${Name}=" "" Value ${NameAndValue})

  string(REGEX REPLACE "[ ]+" "" Name ${Name})
  string(REGEX REPLACE "^[ |\t]+" "" Value ${Value})
  string(REGEX REPLACE "[\t]+" "" Value ${Value})
  string(REGEX REPLACE "[ ]+" ";" Value ${Value})

  # Set the variable
  set(${Name} ${Value})
endforeach()

list(TRANSFORM ISCDBCSRC PREPEND "${FBODBC_SRC_DIR}/IscDbc/")
list(TRANSFORM ODBCJDBCSRC PREPEND "${FBODBC_SRC_DIR}/")
list(TRANSFORM ODBCJDBCSETUPSRC_LINUX PREPEND "${FBODBC_SRC_DIR}/")

add_library(objlib OBJECT ${ISCDBCSRC} ${ODBCJDBCSRC} ${ODBCJDBCSETUPSRC_LINUX})
set_property(TARGET objlib PROPERTY POSITION_INDEPENDENT_CODE 1)
target_compile_features(objlib PUBLIC cxx_std_17)

add_library(OdbcFb       SHARED $<TARGET_OBJECTS:objlib>)
add_library(OdbcFbStatic STATIC $<TARGET_OBJECTS:objlib>)

target_link_libraries(OdbcFb       PRIVATE dl PRIVATE crypt PRIVATE ODBC::ODBC)
target_link_libraries(OdbcFbStatic PRIVATE dl PRIVATE crypt PRIVATE ODBC::ODBC)
