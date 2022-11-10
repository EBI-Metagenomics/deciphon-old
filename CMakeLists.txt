cmake_minimum_required(VERSION 3.21 FATAL_ERROR)

project(
  deciphon
  VERSION 0.1.0
  LANGUAGES C)

include(CheckSymbolExists)
include(cmake/compiler-options.cmake)
include(cmake/sanitizers.cmake)
include(cmake/CPM.cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

cpmaddpackage("gh:EBI-Metagenomics/h3client@0.4.0")
cpmaddpackage("gh:EBI-Metagenomics/hmmer-reader@0.2.1")
cpmaddpackage("gh:EBI-Metagenomics/imm@2.0.10")
cpmaddpackage("gh:EBI-Metagenomics/lite-pack@0.4.0")

find_package(CURL REQUIRED)
find_package(OpenMP REQUIRED)
find_package(Threads REQUIRED)
find_package(LibUV REQUIRED)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_EXTENSIONS OFF)

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
  libs/fs.c
  libs/jx.c
  libs/lazylog.c
  libs/zc.c
  src/core/as.c
  src/core/buff.c
  src/core/command_help.c
  src/core/errmsg.c
  src/core/expect.c
  src/core/file.c
  src/core/fmt.c
  src/core/global.c
  src/core/is.c
  src/core/machine.c
  src/core/mempool.c
  src/core/msg.c
  src/core/multifile.c
  src/core/pidfile.c
  src/core/progress.c
  src/core/rc.c
  src/core/sched.c
  src/core/sched_dump.c
  src/core/sharg.c
  src/core/to.c
  src/core/url.c
  src/core/xmath.c
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
  src/loop/parent.c
  src/loop/proc.c
  src/loop/reader.c
  src/loop/stdpipe.c
  src/loop/timer.c
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
  src/scan/prodman.c
  src/scan/protein_match.c
  src/scan/scan.c
  src/scan/seq.c
  src/scan/seqlist.c
  src/scan/thread.c)
add_library(DECIPHON::deciphon ALIAS deciphon)

target_link_libraries(deciphon PRIVATE H3C::h3c)
target_link_libraries(deciphon PRIVATE CURL::libcurl)
target_link_libraries(deciphon PUBLIC HMR::hmr)
target_link_libraries(deciphon PUBLIC IMM::imm)
target_link_libraries(deciphon PUBLIC LITE_PACK::lite_pack)
target_link_libraries(deciphon PRIVATE OpenMP::OpenMP_C)
target_link_libraries(deciphon PRIVATE ${LIBUV_LIBRARIES})

target_compile_options(deciphon PRIVATE ${WARNING_FLAGS})

target_include_directories(
  deciphon
  PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
         $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs>
         $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src> ${LIBUV_INCLUDE_DIRS}
  PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs>)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  include(CheckIPOSupported)
  check_ipo_supported()
  set_target_properties(deciphon PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

add_executable(schedy schedy/api.c schedy/command.c schedy/http.c
                      schedy/schedy.c schedy/xcurl.c schedy/xcurl_debug.c)
target_link_libraries(schedy PRIVATE deciphon)

add_executable(pressy pressy/pressy.c pressy/command.c pressy/presser.c)
target_link_libraries(pressy PRIVATE deciphon)

add_executable(scanny scanny/scanny.c scanny/command.c scanny/scanner.c)
target_link_libraries(scanny PRIVATE deciphon)

add_executable(hmmy hmmy/hmmy.c hmmy/command.c hmmy/hmmer.c hmmy/client.c
                    hmmy/server.c hmmy/boot.c)
target_link_libraries(hmmy PRIVATE deciphon)
add_dependencies(hmmy H3C::h3client)
add_custom_command(
  TARGET hmmy
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:H3C::h3client>
          $<TARGET_FILE_DIR:hmmy>)

add_executable(decy decy/decy.c decy/command.c decy/cfg.c decy/broker.c)
target_link_libraries(decy PRIVATE deciphon)

enable_testing()
add_subdirectory(test)
