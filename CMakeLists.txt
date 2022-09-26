cmake_minimum_required(VERSION 3.16.3 FATAL_ERROR)

project(
  deciphon
  VERSION 0.1.0
  LANGUAGES C)

include(CheckSymbolExists)
include(cmake/compiler-options.cmake)
include(cmake/sanitizers.cmake)
include(cmake/CPM.cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

cpmaddpackage("gh:EBI-Metagenomics/hmmer-reader@0.1.4")
cpmaddpackage("gh:EBI-Metagenomics/imm@2.0.7")
cpmaddpackage("gh:EBI-Metagenomics/lite-pack@0.3.1")
cpmaddpackage(
  NAME
  libuv
  VERSION
  1.44.2
  GITHUB_REPOSITORY
  libuv/libuv
  OPTIONS
  "LIBUV_BUILD_TESTS OFF"
  "LIBUV_BUILD_BENCH OFF")

find_package(CURL REQUIRED)
find_package(OpenMP REQUIRED)
find_package(Threads REQUIRED)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)

option(DEBUG_HTTP_REQUESTS "Debug HTTP requests" OFF)
if(DEBUG_HTTP_REQUESTS)
  message(STATUS "Debug HTTP requests enabled")
else()
  message(STATUS "Debug HTTP requests disabled")
  add_compile_definitions(NDEBUG_HTTP_REQUESTS=1)
endif()

add_compile_definitions(_XOPEN_SOURCE=700)
add_compile_definitions(_DARWIN_C_SOURCE=1)
if(OpenMP_FOUND)
  add_compile_definitions(DECIPHON_OPENM=1)
endif()

add_library(
  deciphon
  # src/decy.c
  # src/benchy.c
  # src/schedy.c
  # src/hmmy.c
  libs/jx.c
  libs/argless.c
  libs/dotenv.c
  libs/zc.c
  libs/xfile.c
  src/core/buff.c
  src/loop/io.c
  # src/core/buff_pool.c
  src/core/expect.c
  src/core/http.c
  # src/core/line.c
  src/core/file.c
  src/core/cmd.c
  src/loop/looper.c
  src/loop/loopio.c
  src/loop/reader.c
  src/loop/writer.c
  src/core/logging.c
  src/core/mempool.c
  src/core/errmsg.c
  src/core/progress.c
  src/core/to.c
  src/scan/prod.c
  src/scan/protein_match.c
  src/scan/scan.c
  src/scan/seq.c
  src/scan/seqlist.c
  src/scan/thread.c
  src/model/nuclt_dist.c
  src/model/profile.c
  src/model/profile_types.c
  src/model/protein_codec.c
  src/model/protein_h3reader.c
  src/model/protein_model.c
  src/model/protein_profile.c
  src/model/protein_state.c
  src/model/standard_profile.c
  src/model/standard_state.c
  src/db/reader.c
  src/db/writer.c
  src/db/press.c
  src/db/protein_writer.c
  src/db/protein_reader.c
  src/db/profile_reader.c
  src/core/api.c
  src/core/sched_dump.c
  src/core/sched.c
  src/core/url.c
  src/core/xcurl.c
  src/core/xcurl_debug.c)
# src/server/hmm.c src/server/job.c src/server/job_state.c src/server/prod.c
# src/server/protein_match.c src/server/scan.c src/server/scan_thread.c)
# src/server/server.c)
add_library(DECIPHON::deciphon ALIAS deciphon)

target_link_libraries(deciphon PUBLIC CURL::libcurl)
target_link_libraries(deciphon PUBLIC HMR::hmr)
target_link_libraries(deciphon PUBLIC IMM::imm)
target_link_libraries(deciphon PUBLIC LITE_PACK::lite_pack)
target_link_libraries(deciphon PUBLIC OpenMP::OpenMP_C)
target_link_libraries(deciphon PUBLIC uv_a)

target_compile_options(deciphon PRIVATE ${WARNING_FLAGS})

# $<INSTALL_INTERFACE:include>
target_include_directories(
  deciphon
  PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
         $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs>
         $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
  PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs>)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  include(CheckIPOSupported)
  check_ipo_supported()
  set_target_properties(deciphon PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

add_executable(schedy src/schedy.c src/schedy_cmd.c)
target_link_libraries(schedy PUBLIC deciphon)

add_executable(pressy src/pressy.c src/pressy_cmd.c src/pressy_session.c)
target_link_libraries(pressy PUBLIC deciphon)

add_executable(scanny src/scanny.c src/scanny_cmd.c src/scanny_session.c)
target_link_libraries(scanny PUBLIC deciphon)

add_executable(decy src/decy.c src/decy_session.c)
target_link_libraries(decy PUBLIC deciphon)

enable_testing()
add_subdirectory(test)

set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VENDOR "Danilo Horta")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "DECIPHON")
set(CPACK_PACKAGE_CONTACT "Danilo Horta")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md)
set(CPACK_RESOURCE_FILE_README ${CMAKE_CURRENT_SOURCE_DIR}/README.md)
set(CPACK_OUTPUT_FILE_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/package)
set(CPACK_VERBATIM_VARIABLES YES)
set(CPACK_PACKAGE_RELOCATABLE YES)
set(CPACK_MONOLITHIC_INSTALL YES)
include(CPack)
