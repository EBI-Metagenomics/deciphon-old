cmake_minimum_required(VERSION 3.18 FATAL_ERROR)
project(
  deciphon
  VERSION 0.0.4
  LANGUAGES C)

# Generate compile_commands.json file
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
# Set RPATH only if it's not a system directory
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES
     "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
if("${isSystemDir}" STREQUAL "-1")
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
  # /wd5105:
  # https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/c5105?view=msvc-160
  set(WARNING_FLAGS /W3 /wd5105)
else()
  set(WARNING_FLAGS
      -Wall
      -Wextra
      -Wstrict-prototypes
      -Wshadow
      -Wconversion
      -Wmissing-prototypes
      -Wno-unused-parameter
      -Wsign-conversion
      -Wno-unused-function)
endif()

if(NOT CMAKE_C_COMPILER_ID STREQUAL "GNU")
  list(APPEND WARNING_FLAGS -Wno-gnu-designator -Wno-empty-translation-unit
       -Wno-gnu-statement-expression -Wno-nullability-extension)
endif()

option(CI_BUILD "CI, extra flags will be set" OFF)
if(CI_BUILD)
  message(STATUS "CI build enabled")
  if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/WX)
  else()
    add_compile_options(-Werror)
  endif()
endif()

find_package(Argp REQUIRED)
find_package(Threads REQUIRED)
find_package(athr REQUIRED)
find_package(fasta REQUIRED)
find_package(hmr REQUIRED)
find_package(imm REQUIRED)
find_package(log REQUIRED)

option(OPENMP "Enable OpenMP" On)
if(OPENMP)
  find_package(OpenMP)
endif()

add_library(
  dcp
  src/cli.c
  src/cli_info.c
  src/cli_press.c
  src/cli_scan.c
  src/cmp.c
  src/db.c
  src/fcopy.c
  src/log.c
  src/meta.c
  src/model.c
  src/nuclt_dist.c
  src/path.c
  src/pro_codec.c
  src/pro_db.c
  src/pro_model.c
  src/pro_prof.c
  src/pro_reader.c
  src/pro_state.c
  src/progress_file.c
  src/std_db.c
  src/std_prof.c
  src/third-party/cmp.c)
add_library(DCP::dcp ALIAS dcp)

# Hide functions by default.
set_property(TARGET dcp PROPERTY C_VISIBILITY_PRESET hidden)
set_property(TARGET dcp PROPERTY VISIBILITY_INLINES_HIDDEN ON)

# Write out dcp/export.h to the current binary directory.
include(GenerateExportHeader)
generate_export_header(
  dcp
  BASE_NAME
  DCP
  INCLUDE_GUARD_NAME
  DCP_EXPORT_H
  EXPORT_MACRO_NAME
  DCP_API
  EXPORT_FILE_NAME
  ${CMAKE_CURRENT_BINARY_DIR}/dcp/export.h)

target_include_directories(
  dcp
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
         # $<BUILD_INTERFACE:/Users/horta/code/hmmer-eddy/src>
         # $<BUILD_INTERFACE:/Users/horta/code/hmmer-eddy/easel>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
# target_link_libraries(dcp PUBLIC hmmer) target_link_libraries(dcp PUBLIC
# easel)
target_link_libraries(dcp PUBLIC $<$<NOT:$<BOOL:${ARGP_BUILTIN}>>:Argp::argp>)
target_link_libraries(dcp PUBLIC Threads::Threads)
target_link_libraries(dcp PUBLIC ATHR::athr)
target_link_libraries(dcp PUBLIC FASTA::fasta)
target_link_libraries(dcp PUBLIC HMR::hmr)
target_link_libraries(dcp PUBLIC IMM::imm)
target_link_libraries(dcp PUBLIC LOG::log)
target_link_libraries(dcp PRIVATE $<$<BOOL:${OpenMP_C_FOUND}>:OpenMP::OpenMP_C>)
set_target_properties(dcp PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(dcp PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})
target_compile_features(dcp PRIVATE c_std_11)
target_compile_definitions(
  dcp PRIVATE $<$<BOOL:${WIN32}>:_CRT_NONSTDC_NO_DEPRECATE>)
target_compile_definitions(dcp PRIVATE $<$<BOOL:${WIN32}>:__extension__>)
target_compile_options(dcp PRIVATE ${WARNING_FLAGS})
target_compile_options(dcp PRIVATE "-fverbose-asm")
target_link_libraries(dcp INTERFACE $<$<BOOL:${UNIX}>:m>)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/dcp/version.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/dcp/version.h @ONLY)

include(CheckCSourceCompiles)
include(CheckTypeSize)

check_c_source_compiles(
  "
        static void f(char const* err, ...) __attribute__((format(printf, 1, 2)));
        int main(void) {return 0;}
    "
  HAVE_ATTR_FMT)
target_compile_definitions(dcp
                           PRIVATE $<$<BOOL:${HAVE_ATTR_FMT}>:HAVE_ATTR_FMT>)

check_type_size("int" SIZEOF_INT)
if(NOT HAVE_SIZEOF_INT)
  message(FATAL_ERROR "Could not determine the size of int.")
endif()
math(EXPR BITS_PER_INT "${SIZEOF_INT} * 8")
target_compile_definitions(dcp PRIVATE BITS_PER_INT=${BITS_PER_INT})

# Command line interface programs
add_executable(dcp-press src/bin/press.c)
target_link_libraries(dcp-press PUBLIC dcp)

add_executable(dcp-info src/bin/info.c)
target_link_libraries(dcp-info PUBLIC dcp)

add_executable(dcp-scan src/bin/scan.c)
target_link_libraries(dcp-scan PUBLIC dcp)

# add_executable(hmmer3 src/hmmer3.c) target_link_libraries(hmmer3 PUBLIC dcp)

# Installation
install(
  TARGETS dcp
  EXPORT dcp-targets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  PUBLIC_HEADER DESTINATION include)
install(DIRECTORY src/dcp/ DESTINATION include/dcp)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dcp/export.h
              ${CMAKE_CURRENT_BINARY_DIR}/dcp/version.h DESTINATION include/dcp)
install(
  EXPORT dcp-targets
  FILE dcp-targets.cmake
  NAMESPACE DCP::
  DESTINATION lib/cmake/dcp)
include(CMakePackageConfigHelpers)
configure_package_config_file(
  dcp-config.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/dcp-config.cmake
  INSTALL_DESTINATION lib/cmake/dcp
  NO_SET_AND_CHECK_MACRO)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dcp-config.cmake
        DESTINATION lib/cmake/dcp)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/dcp-info
        ${CMAKE_CURRENT_BINARY_DIR}/dcp-press
        ${CMAKE_CURRENT_BINARY_DIR}/dcp-scan
        # ${CMAKE_CURRENT_BINARY_DIR}/hmmer3
  PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
  DESTINATION bin)

enable_testing()
add_subdirectory(test)
if(FUZZ)
  add_subdirectory(fuzz)
endif()
