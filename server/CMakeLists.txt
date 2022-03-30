add_library(
  server
  job.c
  job_state.c
  prod.c
  protein_match.c
  scan.c
  scan_thread.c
  server.c)

target_link_libraries(server PUBLIC OpenMP::OpenMP_C)
