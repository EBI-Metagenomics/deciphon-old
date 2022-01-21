project(
  sched
  VERSION 0.0.1
  LANGUAGES C
)

set(SQLite3_USE_STATIC_LIBS ON)
find_package(SQLite3 3.31.1 REQUIRED)

find_program(XXD_FOUND "xxd")
if(XXD_FOUND)
  message(STATUS "Looking for xxd - found")
else()
  message(FATAL_ERROR "Looking for xxd - not found")
endif()

add_custom_command(
  OUTPUT schema.c
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/embed-file
  ${CMAKE_CURRENT_SOURCE_DIR}/src/schema.sql
  schema
  schema.c
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/embed-file
  ${CMAKE_CURRENT_SOURCE_DIR}/src/schema.sql
  VERBATIM
)

add_library(
  ${PROJECT_NAME} STATIC
  schema.c
  src/db.c
  src/job.c
  src/prod.c
  src/sched.c
  src/seq.c
  src/seq_queue.c
  src/stmt.c
  src/tok.c
  src/xsql.c
)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

target_link_libraries(${PROJECT_NAME} PUBLIC SQLite::SQLite3)
target_link_libraries(${PROJECT_NAME} PUBLIC DECIPHON::common)

cbp_bundle_static_library(${PROJECT_NAME} ${PROJECT_NAME}_bundled)
add_library(DECIPHON::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
add_library(DECIPHON::${PROJECT_NAME}_bundled ALIAS ${PROJECT_NAME}_bundled)

# add_subdirectory(test)
