add_library(
  server
  api.c
  job.c
  job_state.c
  prod.c
  protein_match.c
  scan.c
  scan_thread.c
  sched.c
  server.c
  url.c
  xcurl.c
  xcurl_debug.c
  xjson.c)

target_link_libraries(server PUBLIC CURL::libcurl)
target_link_libraries(server PUBLIC OpenMP::OpenMP_C)
