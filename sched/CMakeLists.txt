add_library(
  sched
  api.c
  jsmn.c
  sched.c
  url.c
  xcurl.c
  xcurl_debug.c
  xjson.c)

target_link_libraries(sched PUBLIC CURL::libcurl)
target_link_libraries(sched PUBLIC C_TOOLBELT::c_toolbelt)
