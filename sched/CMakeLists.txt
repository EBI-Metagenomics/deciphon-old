project(sched)

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
  src/tok.c
  src/xsql.c
)
add_library(DECIPHON::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PUBLIC DECIPHON::common)
target_link_libraries(${PROJECT_NAME} PRIVATE SQLite::SQLite3)

add_subdirectory(test)
