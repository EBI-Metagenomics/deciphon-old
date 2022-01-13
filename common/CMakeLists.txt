project(common)

set(xxHash_USE_STATIC_LIBS ON)
find_package(xxHash REQUIRED)

add_library(
  ${PROJECT_NAME} STATIC
  src/bitops.c
  src/logger.c
  src/safe.c
  src/to.c
  src/xfile.c
)
add_library(DECIPHON::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

include(GenerateExportHeader)
generate_export_header(
  ${PROJECT_NAME}
  EXPORT_MACRO_NAME EXPORT
  EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/common/export.h
  INCLUDE_GUARD_NAME EXPORT_H
)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

target_link_libraries(${PROJECT_NAME} PUBLIC xxHash::xxHash)
cbp_bundle_static_library(${PROJECT_NAME} ${PROJECT_NAME}_bundled)

add_subdirectory(test)
