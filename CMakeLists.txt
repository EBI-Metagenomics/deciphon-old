cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
project(
  deciphon
  VERSION 0.0.1
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

find_package(elapsed REQUIRED)
find_package(imm REQUIRED)
find_package(Threads REQUIRED)

option(OPENMP "Enable OpenMP" On)
if(OPENMP)
  find_package(OpenMP)
endif()

add_library(
  dcp
  # src/clock.c
  # src/dthread.c
  src/db.c
  src/support.c
  src/model.c
  # src/fifo.c
  src/fifo1.c
  # src/input.c
  # src/mpool.c
  # src/output.c
  src/profile.c
  src/normal_profile.c
  src/pro_profile.c
  # src/result.c
  # src/result_ring.c
  # src/results.c
  # src/scan.c
  # src/seq_stack.c
  src/server.c
  # src/sleep.c
  # src/special_trans.c
  # src/str.c
  # src/task.c
  # src/task_bin.c
  # src/util.c
  third-party/cmp.c)
add_library(DCP::dcp ALIAS dcp)

# Write out mytools_export.h to the current binary directory Must include
# ${CMAKE_CURRENT_BINARY_DIR} in the target's search path.
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
  add_compile_options(-Werror)
endif()

target_include_directories(
  dcp
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
          ${CMAKE_CURRENT_SOURCE_DIR}/third-party)
target_link_libraries(dcp PUBLIC IMM::imm)
target_link_libraries(dcp PUBLIC ELAPSED::elapsed)
target_link_libraries(dcp PRIVATE Threads::Threads)
target_link_libraries(dcp PUBLIC ck)
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
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/dcp/config.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/dcp/config.h @ONLY)

include(CheckCSourceCompiles)
check_c_source_compiles(
  "
        static void f(char const* err, ...) __attribute__((format(printf, 1, 2)));
        int main(void) {return 0;}
    "
  HAVE_ATTR_FMT)
target_compile_definitions(dcp
                           PRIVATE $<$<BOOL:${HAVE_ATTR_FMT}>:HAVE_ATTR_FMT>)

include(CheckTypeSize)

check_type_size("int" SIZEOF_INT)
if(NOT HAVE_SIZEOF_INT)
  message(FATAL_ERROR "Could not determine the size of int.")
endif()
math(EXPR BITS_PER_INT "${SIZEOF_INT} * 8")
target_compile_definitions(dcp PRIVATE BITS_PER_INT=${BITS_PER_INT})

install(
  TARGETS dcp
  EXPORT dcp-targets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  PUBLIC_HEADER DESTINATION include)
install(DIRECTORY include/ DESTINATION include)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dcp/export.h
              ${CMAKE_CURRENT_BINARY_DIR}/dcp/config.h DESTINATION include/dcp)
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

enable_testing()
add_subdirectory(test)
if(FUZZ)
  add_subdirectory(fuzz)
endif()
