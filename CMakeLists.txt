cmake_minimum_required(VERSION 3.18 FATAL_ERROR)
project(
  deciphon
  VERSION 0.0.4
  LANGUAGES C)

# Generate compile_commands.json file
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include("${CMAKE_CURRENT_LIST_DIR}/external/cmake-boilerplate/cbp.cmake")
cbp_set_rpath()

find_package(Argp REQUIRED)
find_package(Threads REQUIRED)
find_package(athr REQUIRED)
find_package(fasta REQUIRED)
find_package(hmr REQUIRED)
find_package(imm REQUIRED)
find_package(log REQUIRED)
find_package(gff REQUIRED)
find_package(tbl REQUIRED)
find_package(SQLite3 3.35.0 REQUIRED)
find_package(PkgConfig REQUIRED)

add_library(
  dcp
  src/cli.c
  src/cli_info.c
  src/cli_press.c
  src/cli_scan.c
  src/cmp.c
  src/db.c
  src/dcp_file.c
  src/log.c
  src/meta.c
  src/model.c
  src/nuclt_dist.c
  src/path.c
  src/pro_codec.c
  src/pro_db.c
  src/pro_match.c
  src/pro_model.c
  src/pro_prod.c
  src/pro_prof.c
  src/pro_reader.c
  src/pro_state.c
  src/prod.c
  src/progress_file.c
  src/row.c
  src/sched.c
  src/sched_job.c
  src/seq.c
  src/server.c
  src/sqldiff.c
  src/std_db.c
  src/std_prof.c
  src/table.c
  src/third-party/base64.c
  src/third-party/cmp.c
  src/xstrlcpy.c)
add_library(DCP::dcp ALIAS dcp)

cbp_hide_functions(dcp)
cbp_set_warning_flags()
cbp_ci_build_option()
cbp_generate_export_header(dcp EXPORT_FILE)
cbp_assert_null_is_zero_bits()

option(OPENMP "Enable OpenMP" On)
if(OPENMP)
  find_package(OpenMP)
endif()

target_include_directories(
  dcp
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(dcp PUBLIC $<$<NOT:$<BOOL:${ARGP_BUILTIN}>>:Argp::argp>)
target_link_libraries(dcp PUBLIC Threads::Threads)
target_link_libraries(dcp PUBLIC ATHR::athr)
target_link_libraries(dcp PUBLIC FASTA::fasta)
target_link_libraries(dcp PUBLIC HMR::hmr)
target_link_libraries(dcp PUBLIC IMM::imm)
target_link_libraries(dcp PUBLIC LOG::log)
target_link_libraries(dcp PUBLIC GFF::gff)
target_link_libraries(dcp PUBLIC TBL::tbl)
target_link_libraries(dcp PUBLIC SQLite::SQLite3)
target_link_libraries(dcp PRIVATE $<$<BOOL:${OpenMP_C_FOUND}>:OpenMP::OpenMP_C>)
set_target_properties(dcp PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(dcp PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})
target_compile_features(dcp PRIVATE c_std_11)
target_compile_definitions(
  dcp PRIVATE $<$<BOOL:${WIN32}>:_CRT_NONSTDC_NO_DEPRECATE>)
target_compile_definitions(dcp PRIVATE $<$<BOOL:${WIN32}>:__extension__>)
target_compile_definitions(dcp PRIVATE SQLITE_OMIT_LOAD_EXTENSION)
target_compile_options(dcp PRIVATE ${WARNING_FLAGS})
target_compile_options(dcp PRIVATE "-fverbose-asm")
target_link_libraries(dcp INTERFACE $<$<BOOL:${UNIX}>:m>)

set(VERSION_FILE ${CMAKE_CURRENT_BINARY_DIR}/dcp/version.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/dcp/version.h.in ${VERSION_FILE}
               @ONLY)

target_compile_definitions(dcp PRIVATE _XOPEN_SOURCE=1)
set(CMAKE_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS};-D_XOPEN_SOURCE=1)

# Command line interface programs
add_executable(dcp-press src/bin/press.c)
target_link_libraries(dcp-press PUBLIC dcp)

add_executable(dcp-info src/bin/info.c)
target_link_libraries(dcp-info PUBLIC dcp)

add_executable(dcp-scan src/bin/scan.c)
target_link_libraries(dcp-scan PUBLIC dcp)

cbp_install(dcp MERGED)
install(FILES ${EXPORT_FILE} ${VERSION_FILE} DESTINATION include/dcp)
install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/dcp-info
        ${CMAKE_CURRENT_BINARY_DIR}/dcp-press
        ${CMAKE_CURRENT_BINARY_DIR}/dcp-scan
  PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
  DESTINATION bin)

enable_testing()
add_subdirectory(test)
if(FUZZ)
  add_subdirectory(fuzz)
endif()
