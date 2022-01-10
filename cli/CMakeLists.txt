project(cli)

find_package(tbl REQUIRED)
find_package(Argp REQUIRED)

add_library(
  ${PROJECT_NAME}
  src/bin_info.c
  src/bin_press.c
  src/bin_scan.c
  src/bin_server.c
  src/cli.c
  src/cli_info.c
  src/cli_press.c
  src/cli_scan.c
)

target_link_libraries(dcp PUBLIC TBL::tbl)
target_link_libraries(dcp PUBLIC $<$<NOT:$<BOOL:${ARGP_BUILTIN}>>:Argp::argp>)

add_executable(dcp-press bin/press.c)
target_link_libraries(dcp-press PUBLIC dcp)

add_executable(dcp-info bin/info.c)
target_link_libraries(dcp-info PUBLIC dcp)
