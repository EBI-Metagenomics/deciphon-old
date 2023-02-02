cmake_minimum_required(VERSION 3.21 FATAL_ERROR)
project(
  deciphon
  VERSION 0.4.1
  LANGUAGES C)

include(cmake/warnings.cmake)
include(cmake/sanitizers.cmake)
include(cmake/CPM.cmake)

cpmaddpackage("gh:EBI-Metagenomics/hmr@0.6.0")
cpmaddpackage("gh:EBI-Metagenomics/imm@2.1.10")
cpmaddpackage("gh:EBI-Metagenomics/lip@0.5.0")
cpmaddpackage("gh:EBI-Metagenomics/h3client@0.10.0")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS OFF)

add_library(
  deciphon
  codec.c
  db_reader.c
  db_writer.c
  expect.c
  fs.c
  h3reader.c
  json.c
  lrt.c
  match.c
  match_iter.c
  model.c
  nuclt_dist.c
  partition_size.c
  press.c
  prod.c
  prod_file.c
  prod_thrd.c
  protein.c
  protein_iter.c
  protein_reader.c
  scan.c
  scan_db.c
  scan_task.c
  scan_thrd.c
  seq_list.c
  state.c
  strerror.c
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

target_link_libraries(deciphon PUBLIC HMR::hmr)
target_link_libraries(deciphon PUBLIC IMM::imm)
target_link_libraries(deciphon PUBLIC LIP::lip)

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
