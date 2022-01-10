cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

project(
  deciphon
  VERSION 0.0.4
  LANGUAGES C
)

# Generate compile_commands.json file
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include("${CMAKE_CURRENT_LIST_DIR}/external/cmake-boilerplate/cbp.cmake")
cbp_set_rpath()
cbp_include_sanitizers()
cbp_include_code_coverage()
add_code_coverage_all_targets()
cbp_set_warning_flags()
cbp_ci_build_option()
cbp_assert_null_is_zero_bits()
enable_testing()

add_subdirectory(common)
add_subdirectory(sched)
# add_subdirectory(core)
# add_subdirectory(cli)

# target_code_coverage(dcp)
# cbp_hide_functions(dcp)

# target_code_coverage(sched)
# cbp_hide_functions(sched)

# target_code_coverage(cli)
# cbp_hide_functions(cli)

# add_executable(dcp-scan bin/scan.c) target_link_libraries(dcp-scan PUBLIC dcp)
#
# add_executable(dcp-server bin/server.c) target_link_libraries(dcp-server
# PUBLIC dcp)

# install(
#   FILES ${VERSION_FILE} ${CMAKE_CURRENT_BINARY_DIR}/dcp-info
#   ${CMAKE_CURRENT_BINARY_DIR}/dcp-press
#   # ${CMAKE_CURRENT_BINARY_DIR}/dcp-scan
#   # ${CMAKE_CURRENT_BINARY_DIR}/dcp-server
#   PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
#   DESTINATION bin
# )

# if(FUZZ) add_subdirectory(fuzz) endif()
