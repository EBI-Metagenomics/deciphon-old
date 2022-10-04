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

if(OpenMP_FOUND)
  add_compile_definitions(DECIPHON_OPENM=1)
endif()

add_library(
  deciphon
  libs/argless.c
  libs/dotenv.c
  libs/jx.c
  libs/lazylog.c
  libs/xfile.c
  libs/zc.c
  src/core/api.c
  src/core/buff.c
  src/core/cmd.c
  src/core/daemonize.c
  src/core/errmsg.c
  src/core/expect.c
  src/core/file.c
  src/core/fmt.c
  src/core/global.c
  src/core/http.c
  src/core/mempool.c
  src/core/pidfile.c
  src/core/progress.c
  src/core/sched.c
  src/core/sched_dump.c
  src/core/str.c
  src/core/to.c
  src/core/url.c
  src/core/xcurl.c
  src/core/xcurl_debug.c
  src/core/xmem.c
  src/db/press.c
  src/db/profile_reader.c
  src/db/protein_reader.c
  src/db/protein_writer.c
  src/db/reader.c
  src/db/writer.c
  src/loop/child.c
  src/loop/input.c
  src/loop/output.c
  src/loop/reader.c
  src/loop/stdpipe.c
  src/loop/writer.c
  src/loop/writer_req.c
  src/loop/writer_req_pool.c
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
  src/scan/prod.c
  src/scan/protein_match.c
  src/scan/scan.c
  src/scan/seq.c
  src/scan/seqlist.c
  src/scan/thread.c)
add_library(DECIPHON::deciphon ALIAS deciphon)

target_link_libraries(deciphon PUBLIC CURL::libcurl)
target_link_libraries(deciphon PUBLIC HMR::hmr)
target_link_libraries(deciphon PUBLIC IMM::imm)
target_link_libraries(deciphon PUBLIC LITE_PACK::lite_pack)
target_link_libraries(deciphon PUBLIC OpenMP::OpenMP_C)
target_link_libraries(deciphon PUBLIC uv_a)

target_compile_options(deciphon PRIVATE ${WARNING_FLAGS})

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

add_executable(schedy src/schedy/schedy.c src/schedy/cmd.c)
target_link_libraries(schedy PUBLIC deciphon)

add_executable(pressy src/pressy/pressy.c src/pressy/cmd.c src/pressy/session.c)
target_link_libraries(pressy PUBLIC deciphon)

add_executable(scanny src/scanny/scanny.c src/scanny/cmd.c src/scanny/session.c)
target_link_libraries(scanny PUBLIC deciphon)

add_executable(decy src/decy/decy.c src/decy/cmd.c src/decy/cfg.c)
target_link_libraries(decy PUBLIC deciphon)
# add_executable(decy src/decy/decy.c src/decy/schedy.c src/decy/cmd.c
# src/decy/cfg.c)

enable_testing()
add_subdirectory(test)
