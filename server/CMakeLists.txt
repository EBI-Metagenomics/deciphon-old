add_library(server prod.c work.c rest.c rest_curl.c protein_match.c server.c job_state.c)

target_link_libraries(server PUBLIC CURL::libcurl)
