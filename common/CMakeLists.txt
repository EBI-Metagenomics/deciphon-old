project(common)

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

target_link_libraries(${PROJECT_NAME} PRIVATE xxHash::xxHash)

add_subdirectory(test)
