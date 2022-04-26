add_library(
  server
  file.c
  hmm.c
  job.c
  job_state.c
  prod.c
  protein_match.c
  scan.c
  scan_thread.c
  server.c)

target_link_libraries(server PUBLIC OpenMP::OpenMP_C)
target_link_libraries(server PUBLIC sched)
target_link_libraries(server PUBLIC core)
target_link_libraries(server PUBLIC model)
target_link_libraries(server PUBLIC db)
