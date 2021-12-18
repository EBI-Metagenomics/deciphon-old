cmake_minimum_required(VERSION 3.18 FATAL_ERROR)
project(
  deciphon
  VERSION 0.0.4
  LANGUAGES C
)

# Generate compile_commands.json file
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include("${CMAKE_CURRENT_LIST_DIR}/external/cmake-boilerplate/cbp.cmake")
cbp_set_rpath()
cbp_include_sanitizers()
cbp_include_code_coverage()
add_code_coverage_all_targets()

find_package(elapsed REQUIRED)
find_package(athr REQUIRED)
find_package(log REQUIRED)
find_package(hmr REQUIRED)
find_package(fasta REQUIRED)
find_package(cmp REQUIRED)
find_package(imm REQUIRED)
find_package(tbl REQUIRED)
find_package(gff REQUIRED)
find_package(dcp_sched REQUIRED)
find_package(Threads REQUIRED)
find_package(Argp REQUIRED)

add_library(
  dcp
  # src/cli_server.c
  src/bitops.c
  src/cli.c
  src/cli_info.c
  src/cli_press.c
  src/cli_scan.c
  src/db.c
  src/db_reader.c
  src/job_state.c
  src/logger.c
  src/nuclt_dist.c
  src/pool.c
  src/prod.c
  src/profile.c
  src/profile_reader.c
  src/profile_types.c
  src/progress_file.c
  src/protein_codec.c
  src/protein_db.c
  src/protein_hmmer3_reader.c
  src/protein_match.c
  src/protein_model.c
  src/protein_profile.c
  src/protein_state.c
  src/row.c
  src/safe.c
  src/seq.c
  src/server.c
  src/standard_db.c
  src/standard_match.c
  src/standard_profile.c
  src/standard_state.c
  src/table.c
  src/to.c
  src/tok.c
  src/work.c
  src/xfile.c
)
add_library(DCP::dcp ALIAS dcp)
target_code_coverage(dcp)

cbp_hide_functions(dcp)
cbp_set_warning_flags()
cbp_ci_build_option()
cbp_assert_null_is_zero_bits()

option(OPENMP "Enable OpenMP" On)
if(OPENMP)
  find_package(OpenMP REQUIRED)
  message(STATUS "OpenMP - Enabled")
else()
  message(STATUS "OpenMP - Disabled")
endif()

target_include_directories(
  dcp
  PUBLIC $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(dcp PUBLIC ELAPSED::elapsed)
target_link_libraries(dcp PUBLIC ATHR::athr)
target_link_libraries(dcp PUBLIC LOG::log)
target_link_libraries(dcp PUBLIC HMR::hmr)
target_link_libraries(dcp PUBLIC FASTA::fasta)
target_link_libraries(dcp PUBLIC CMP::cmp)
target_link_libraries(dcp PUBLIC IMM::imm)
target_link_libraries(dcp PUBLIC TBL::tbl)
target_link_libraries(dcp PUBLIC GFF::gff)
target_link_libraries(dcp PUBLIC DCP_SCHED::dcp_sched)
target_link_libraries(dcp PUBLIC Threads::Threads)
target_link_libraries(dcp PUBLIC $<$<NOT:$<BOOL:${ARGP_BUILTIN}>>:Argp::argp>)
target_link_libraries(dcp PRIVATE $<$<BOOL:${OpenMP_C_FOUND}>:OpenMP::OpenMP_C>)
target_link_libraries(dcp INTERFACE $<$<BOOL:${UNIX}>:m>)

set_target_properties(dcp PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(dcp PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})

target_compile_features(dcp PRIVATE c_std_11)

target_compile_definitions(dcp PRIVATE $<$<BOOL:${WIN32}>:_CRT_NONSTDC_NO_DEPRECATE>)
target_compile_definitions(dcp PRIVATE $<$<BOOL:${WIN32}>:__extension__>)
target_compile_definitions(dcp PRIVATE SQLITE_OMIT_LOAD_EXTENSION)
target_compile_definitions(dcp PRIVATE _XOPEN_SOURCE=700)

target_compile_options(dcp PRIVATE ${WARNING_FLAGS})
target_compile_options(dcp PRIVATE "-fverbose-asm")

set(VERSION_FILE ${CMAKE_CURRENT_BINARY_DIR}/version.h)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in ${VERSION_FILE}
  @ONLY
)

# Command line interface programs
add_executable(dcp-press bin/press.c)
target_link_libraries(dcp-press PUBLIC dcp)
#
# add_executable(dcp-info bin/info.c)
# target_link_libraries(dcp-info PUBLIC dcp)
#
# add_executable(dcp-scan bin/scan.c)
# target_link_libraries(dcp-scan PUBLIC dcp)
#
# add_executable(dcp-server bin/server.c)
# target_link_libraries(dcp-server PUBLIC dcp)
#
# install(
#   FILES ${VERSION_FILE}
#   ${CMAKE_CURRENT_BINARY_DIR}/dcp-info
#   ${CMAKE_CURRENT_BINARY_DIR}/dcp-press
#   ${CMAKE_CURRENT_BINARY_DIR}/dcp-scan
#   ${CMAKE_CURRENT_BINARY_DIR}/dcp-server
#   PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
#   DESTINATION bin
# )

enable_testing()
add_subdirectory(test)
# if(FUZZ)
#   add_subdirectory(fuzz)
# endif()
