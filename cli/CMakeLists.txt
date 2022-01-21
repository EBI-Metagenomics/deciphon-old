project(cli VERSION 0.0.1 LANGUAGES C)

# find_package(elapsed REQUIRED)
# find_package(athr REQUIRED)
# find_package(log REQUIRED)
# find_package(hmr REQUIRED)
# find_package(cmp REQUIRED)
# find_package(imm REQUIRED)
# find_package(gff REQUIRED)
# find_package(Threads REQUIRED)
# find_package(CURL REQUIRED)

find_package(tbl REQUIRED)
find_package(Argp REQUIRED)

add_library(
  ${PROJECT_NAME}
  # src/bin_info.c
  src/bin_press.c
  # src/bin_scan.c
  # src/bin_server.c
  src/cli.c
  # src/cli_info.c
  src/cli_press.c
  # src/cli_scan.c
)

target_link_libraries(${PROJECT_NAME} PUBLIC DECIPHON::common)
target_link_libraries(${PROJECT_NAME} PUBLIC DECIPHON::worker)
target_link_libraries(${PROJECT_NAME} PUBLIC TBL::tbl)
target_link_libraries(${PROJECT_NAME} PUBLIC $<$<NOT:$<BOOL:${ARGP_BUILTIN}>>:Argp::argp>)

add_executable(dcp-press src/bin_press.c)
target_link_libraries(dcp-press PUBLIC ${PROJECT_NAME})

# add_executable(dcp-info bin/info.c)
# target_link_libraries(dcp-info PUBLIC ${PROJECT_NAME})












# add_library(DECIPHON::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
#
# target_link_libraries(${PROJECT_NAME} PUBLIC DECIPHON::common)
# target_link_libraries(${PROJECT_NAME} PUBLIC ELAPSED::elapsed)
# target_link_libraries(${PROJECT_NAME} PUBLIC ATHR::athr)
# target_link_libraries(${PROJECT_NAME} PUBLIC LOG::log)
# target_link_libraries(${PROJECT_NAME} PUBLIC HMR::hmr)
# target_link_libraries(${PROJECT_NAME} PUBLIC CMP::cmp)
# target_link_libraries(${PROJECT_NAME} PUBLIC IMM::imm)
# target_link_libraries(${PROJECT_NAME} PUBLIC GFF::gff)
# target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads)
# target_link_libraries(${PROJECT_NAME} PRIVATE $<$<BOOL:${OpenMP_C_FOUND}>:OpenMP::OpenMP_C>)
# target_link_libraries(${PROJECT_NAME} INTERFACE $<$<BOOL:${UNIX}>:m>)
# target_link_libraries(${PROJECT_NAME} PUBLIC CURL::libcurl)
#
# target_compile_features(${PROJECT_NAME} PRIVATE c_std_11)
#
# target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<BOOL:${WIN32}>:_CRT_NONSTDC_NO_DEPRECATE>)
# target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<BOOL:${WIN32}>:__extension__>)
# target_compile_definitions(${PROJECT_NAME} PRIVATE SQLITE_OMIT_LOAD_EXTENSION)
# target_compile_definitions(${PROJECT_NAME} PRIVATE _XOPEN_SOURCE=700)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_options(${PROJECT_NAME} PRIVATE ${WARNING_FLAGS})
target_compile_options(${PROJECT_NAME} PRIVATE "-fverbose-asm")

# add_subdirectory(test)
