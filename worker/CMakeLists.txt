project(worker)

find_package(elapsed REQUIRED)
find_package(athr REQUIRED)
find_package(log REQUIRED)
find_package(hmr REQUIRED)
find_package(cmp REQUIRED)
find_package(imm REQUIRED)
find_package(gff REQUIRED)
find_package(Threads REQUIRED)
find_package(CURL REQUIRED)

option(OPENMP "Enable OpenMP" On)
if(OPENMP)
  find_package(OpenMP REQUIRED)
  message(STATUS "OpenMP - Enabled")
else()
  message(STATUS "OpenMP - Disabled")
endif()

add_library(
  ${PROJECT_NAME}
  src/db.c
  src/db_reader.c
  src/job_state.c
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
  src/seq.c
  # src/server.c
  src/standard_db.c
  src/standard_match.c
  src/standard_profile.c
  src/standard_state.c
  src/table.c
  src/to.c
  src/tok.c
  # src/work.c
  src/rest.c
)

set(VERSION_FILE ${CMAKE_CURRENT_BINARY_DIR}/version.h)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in ${VERSION_FILE}
  @ONLY
)

add_library(DECIPHON::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PUBLIC DECIPHON::common)
target_link_libraries(${PROJECT_NAME} PUBLIC ELAPSED::elapsed)
target_link_libraries(${PROJECT_NAME} PUBLIC ATHR::athr)
target_link_libraries(${PROJECT_NAME} PUBLIC LOG::log)
target_link_libraries(${PROJECT_NAME} PUBLIC HMR::hmr)
target_link_libraries(${PROJECT_NAME} PUBLIC CMP::cmp)
target_link_libraries(${PROJECT_NAME} PUBLIC IMM::imm)
target_link_libraries(${PROJECT_NAME} PUBLIC GFF::gff)
target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads)
target_link_libraries(${PROJECT_NAME} PRIVATE $<$<BOOL:${OpenMP_C_FOUND}>:OpenMP::OpenMP_C>)
target_link_libraries(${PROJECT_NAME} INTERFACE $<$<BOOL:${UNIX}>:m>)
target_link_libraries(${PROJECT_NAME} PUBLIC CURL::libcurl)

target_compile_features(${PROJECT_NAME} PRIVATE c_std_11)

target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<BOOL:${WIN32}>:_CRT_NONSTDC_NO_DEPRECATE>)
target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<BOOL:${WIN32}>:__extension__>)
target_compile_definitions(${PROJECT_NAME} PRIVATE SQLITE_OMIT_LOAD_EXTENSION)
target_compile_definitions(${PROJECT_NAME} PRIVATE _XOPEN_SOURCE=700)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_options(${PROJECT_NAME} PRIVATE ${WARNING_FLAGS})
target_compile_options(${PROJECT_NAME} PRIVATE "-fverbose-asm")

add_subdirectory(test)
