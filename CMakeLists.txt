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

cpmaddpackage(NAME hmr GIT_TAG 0.1.2 GITHUB_REPOSITORY
              EBI-Metagenomics/hmmer-reader)
cpmaddpackage(NAME imm VERSION 2.0.0 GITHUB_REPOSITORY EBI-Metagenomics/imm)
cpmaddpackage(NAME c_toolbelt VERSION 0.2.1 GITHUB_REPOSITORY horta/c-toolbelt)
cpmaddpackage(NAME lite_pack VERSION 0.1.4 GITHUB_REPOSITORY
              EBI-Metagenomics/lite-pack)
cpmaddpackage(NAME sched VERSION 0.4.2 GITHUB_REPOSITORY
              EBI-Metagenomics/deciphon-sched)
cpmaddpackage(NAME dotenv VERSION 0.1.0 GITHUB_REPOSITORY
              EBI-Metagenomics/dotenv)

find_package(Argp REQUIRED)
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
  src/core/buff.c
  src/core/http.c
  src/core/ljson.c
  src/core/logging.c
  src/core/progress.c
  src/core/to.c
  src/core/xfile.c
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
  src/db/protein_writer.c
  src/db/protein_reader.c
  src/db/profile_reader.c
  src/sched/api.c
  src/sched/jsmn.c
  src/sched/sched.c
  src/sched/url.c
  src/sched/xcurl.c
  src/sched/xcurl_debug.c
  src/sched/xjson.c
  src/server/file.c
  src/server/hmm.c
  src/server/job.c
  src/server/job_state.c
  src/server/prod.c
  src/server/protein_match.c
  src/server/scan.c
  src/server/scan_thread.c
  src/server/server.c
  src/cli/cli.c
  src/cli/cli_server.c)
add_library(DECIPHON::deciphon ALIAS deciphon)

target_link_libraries(deciphon PUBLIC CURL::libcurl)
target_link_libraries(deciphon PUBLIC C_TOOLBELT::c_toolbelt)
target_link_libraries(deciphon PUBLIC HMR::hmr)
target_link_libraries(deciphon PUBLIC IMM::imm)
target_link_libraries(deciphon PUBLIC LITE_PACK::lite_pack)
target_link_libraries(deciphon PUBLIC OpenMP::OpenMP_C)
target_link_libraries(deciphon PUBLIC SCHED::headers)
target_link_libraries(deciphon PUBLIC DOTENV::dotenv)

set(VERSION_FILE ${CMAKE_CURRENT_BINARY_DIR}/deciphon/version.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/deciphon/version.h.in
               ${VERSION_FILE} @ONLY)

target_include_directories(
  deciphon
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
         $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
  PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  include(CheckIPOSupported)
  check_ipo_supported()
  set_target_properties(deciphon PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

add_executable(dcp-server src/cli/bin_server.c)
target_link_libraries(dcp-server PUBLIC deciphon)

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
