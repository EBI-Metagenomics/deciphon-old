# add_library(cli cli.c cli_press.c cli_server.c progress_file.c)
add_library(cli cli.c cli_server.c)

# target_link_libraries(cli PUBLIC ATHR::athr)
target_link_libraries(cli PUBLIC server)
target_link_libraries(cli PUBLIC sched)
target_link_libraries(cli PUBLIC core)
target_link_libraries(cli PUBLIC model)
target_link_libraries(cli PUBLIC db)
target_link_libraries(cli PUBLIC $<$<NOT:$<BOOL:${ARGP_BUILTIN}>>:Argp::argp>)

# add_executable(dcp-press bin_press.c) target_link_libraries(dcp-press PUBLIC
# cli)

add_executable(dcp-server bin_server.c)
target_link_libraries(dcp-server PUBLIC cli)
