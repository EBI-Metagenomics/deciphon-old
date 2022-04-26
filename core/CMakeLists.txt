add_library(
  core
  bitops.c
  buff.c
  info.c
  ljson.c
  logger.c
  nanoprintf.c
  rc.c
  str.c
  to.c
  xfile.c)

target_link_libraries(core PUBLIC C_TOOLBELT::c_toolbelt)
