project(sched)

find_package(SQLite3 3.31.1 REQUIRED)

add_library(
  ${PROJECT_NAME} STATIC
  src/db.c
  src/job.c
  src/prod.c
  src/sched.c
  src/seq.c
  src/seq_queue.c
  src/to.c
  src/tok.c
  src/xfile.c
  src/xsql.c
)
add_library(DCP::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PUBLIC DCP::util)
target_link_libraries(${PROJECT_NAME} PUBLIC SQLite::SQLite3)
