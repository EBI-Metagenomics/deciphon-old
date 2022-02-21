cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

project(
  deciphon
  VERSION 0.1.0
  LANGUAGES C)

find_package(CURL REQUIRED)
find_package(Threads REQUIRED)
find_package(elapsed REQUIRED)
find_package(gff REQUIRED)
find_package(hmr REQUIRED)
find_package(imm REQUIRED)
find_package(lite_pack REQUIRED)

macro(set_rpath)
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
  # Set RPATH only if it's not a system directory
  list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES
       "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
  if("${isSystemDir}" STREQUAL "-1")
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
  endif()
endmacro()

set_rpath()

# Generate compile_commands.json file
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(OPENMP "Enable OpenMP" On)
if(OPENMP)
  find_package(OpenMP REQUIRED)
  message(STATUS "OpenMP - Enabled")
else()
  message(STATUS "OpenMP - Disabled")
endif()

add_library(
  ${PROJECT_NAME}
  src/buff.c
  src/db.c
  src/db_mt.c
  src/db_reader.c
  src/db_tmp.c
  # src/job_state.c
  # src/js.c
  # src/nuclt_dist.c
  # src/pool.c
  # src/prod.c
  # src/profile.c
  # src/profile_reader.c
  # src/profile_types.c
  # src/progress_file.c
  # src/protein_codec.c
  # src/protein_db.c
  # src/protein_hmmer3_reader.c
  # src/protein_match.c
  # src/protein_model.c
  # src/protein_profile.c
  # src/protein_state.c
  # src/rest.c
  # src/row.c
  # src/seq.c
  # src/server.c
  # src/standard_db.c
  # src/standard_match.c
  # src/standard_profile.c
  # src/standard_state.c
  # src/table.c
  # src/tok.c
  # src/work.c
  )

add_library(DECIPHON::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(${PROJECT_NAME} PUBLIC CURL::libcurl)
target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads)
target_link_libraries(${PROJECT_NAME} PUBLIC ELAPSED::elapsed)
target_link_libraries(${PROJECT_NAME} PUBLIC GFF::gff)
target_link_libraries(${PROJECT_NAME} PUBLIC HMR::hmr)
target_link_libraries(${PROJECT_NAME} PUBLIC IMM::imm)
target_link_libraries(${PROJECT_NAME} PUBLIC LITE_PACK::lite_pack)
target_link_libraries(${PROJECT_NAME}
                      PRIVATE $<$<BOOL:${OpenMP_C_FOUND}>:OpenMP::OpenMP_C>)
target_link_libraries(${PROJECT_NAME} INTERFACE $<$<BOOL:${UNIX}>:m>)
# target_link_libraries(${PROJECT_NAME} PUBLIC LOG::log)

target_compile_features(${PROJECT_NAME} PRIVATE c_std_11)

target_compile_definitions(
  ${PROJECT_NAME} PRIVATE $<$<BOOL:${WIN32}>:_CRT_NONSTDC_NO_DEPRECATE>)
target_compile_definitions(${PROJECT_NAME}
                           PRIVATE $<$<BOOL:${WIN32}>:__extension__>)
target_compile_definitions(${PROJECT_NAME} PRIVATE _XOPEN_SOURCE=700)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  include(CheckIPOSupported)
  check_ipo_supported()
  set_target_properties(${PROJECT_NAME} PROPERTIES INTERPROCEDURAL_OPTIMIZATION
                                                   TRUE)
endif()

# enable_testing()
# add_subdirectory(test)
