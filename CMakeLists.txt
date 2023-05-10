cmake_minimum_required(VERSION 3.21 FATAL_ERROR)
project(
  deciphon
  VERSION 0.8.4
  LANGUAGES C)

include(cmake/warnings.cmake)
include(cmake/sanitizers.cmake)
include(cmake/CPM.cmake)

cpmaddpackage("gh:EBI-Metagenomics/hmr@0.6.0")
cpmaddpackage("gh:EBI-Metagenomics/imm@3.0.7")
cpmaddpackage("gh:EBI-Metagenomics/lip@0.5.1")
cpmaddpackage("gh:EBI-Metagenomics/h3c@0.10.6")
find_package(OpenMP)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS OFF)

add_library(
  deciphon
  chararray.c
  codec.c
  db_reader.c
  db_writer.c
  expect.c
  fmt.c
  fs.c
  h3reader.c
  hmmer.c
  hmmer_dialer.c
  hmmer_result.c
  iseq.c
  lrt.c
  match.c
  match_iter.c
  model.c
  nuclt_dist.c
  partition_size.c
  press.c
  prod_match.c
  prod_writer.c
  prod_writer_thrd.c
  protein.c
  protein_alts.c
  protein_iter.c
  protein_null.c
  protein_reader.c
  scan.c
  scan_db.c
  scan_task.c
  scan_thrd.c
  seq.c
  seq_iter.c
  state.c
  strerror.c
  strkcpy.c
  strlcpy.c
  u16toa.c
  xtrans.c)
add_library(DECIPHON::deciphon ALIAS deciphon)

if(NOT BUILD_SHARED_LIBS)
  target_compile_definitions(deciphon PUBLIC -DDECIPHON_STATIC_DEFINE)
endif()

target_include_directories(
  deciphon
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
         $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
  PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>)

set_target_properties(deciphon PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(deciphon PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})
target_compile_options(deciphon PRIVATE ${WARNING_FLAGS})

target_link_libraries(deciphon PUBLIC H3C::h3c)
target_link_libraries(deciphon PUBLIC HMR::hmr)
target_link_libraries(deciphon PUBLIC IMM::imm)
target_link_libraries(deciphon PUBLIC LIP::lip)
if(OpenMP_C_FOUND)
  target_link_libraries(deciphon PUBLIC OpenMP::OpenMP_C)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  include(CheckIPOSupported)
  check_ipo_supported()
  set_target_properties(deciphon PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

install(
  TARGETS deciphon
  EXPORT deciphon-targets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
install(DIRECTORY include/deciphon/ DESTINATION include/deciphon)
install(FILES ${CONFIG_FILE} DESTINATION include/deciphon)

export(
  TARGETS deciphon
  NAMESPACE DECIPHON::
  FILE deciphon-targets.cmake)
install(
  EXPORT deciphon-targets
  NAMESPACE DECIPHON::
  DESTINATION lib/cmake/deciphon)

install(FILES ${PROJECT_SOURCE_DIR}/README.md ${PROJECT_SOURCE_DIR}/LICENSE
        DESTINATION share/docs/deciphon)

include(CMakePackageConfigHelpers)

set(project_config ${PROJECT_BINARY_DIR}/deciphon-config.cmake)
set(version_config ${PROJECT_BINARY_DIR}/deciphon-config-version.cmake)

configure_package_config_file(deciphon-config.cmake.in ${project_config}
                              INSTALL_DESTINATION lib/cmake/deciphon)

write_basic_package_version_file(${version_config}
                                 COMPATIBILITY SameMajorVersion)

install(FILES ${project_config} ${version_config}
        DESTINATION lib/cmake/deciphon)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  enable_testing()
  add_subdirectory(test)
endif()
