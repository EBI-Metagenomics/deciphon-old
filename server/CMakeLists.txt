add_library(
  server
  job_state.c
  prod.c
  protein_match.c
  rest.c
  server.c
  url.c
  work.c
  xcurl.c
  xjson.c)

target_link_libraries(server PUBLIC CURL::libcurl)
