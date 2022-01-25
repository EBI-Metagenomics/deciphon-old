project(
  common
  VERSION 0.0.1
  LANGUAGES C
)

set(xxHash_USE_STATIC_LIBS ON)
find_package(xxHash REQUIRED)

add_library(
  ${PROJECT_NAME} STATIC
  src/bitops.c
  src/logger.c
  src/safe.c
  src/to.c
  src/rc.c
  src/xfile.c
)

include(GenerateExportHeader)
add_library(DECIPHON::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
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
target_compile_definitions(${PROJECT_NAME} PRIVATE _XOPEN_SOURCE=700)
target_compile_definitions(${PROJECT_NAME} PRIVATE _DARWIN_C_SOURCE)

add_subdirectory(test)
