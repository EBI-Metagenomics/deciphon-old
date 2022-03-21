add_library(
  server
  job_state.c
  prod.c
  protein_match.c
  sched.c
  sched_api.c
  server.c
  thread.c
  url.c
  work.c
  xcurl.c
  xcurl_debug.c
  xjson.c)

target_link_libraries(server PUBLIC CURL::libcurl)
target_link_libraries(server PUBLIC OpenMP::OpenMP_C)
