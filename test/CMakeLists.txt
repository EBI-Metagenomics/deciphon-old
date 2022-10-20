set(TMPDIR "${CMAKE_CURRENT_BINARY_DIR}/tmp")
file(MAKE_DIRECTORY "${TMPDIR}")

function(deciphon_add_test name)
  add_executable(${name} ${name}.c)
  target_link_libraries(${name} PRIVATE DECIPHON::deciphon)
  target_compile_definitions(${name} PRIVATE TMPDIR="${TMPDIR}")
  add_test(NAME test_${name} COMMAND ${name})
endfunction()

deciphon_add_test(standard_profile)
deciphon_add_test(protein_model)
deciphon_add_test(protein_profile)
deciphon_add_test(protein_db)

set(SRC ${CMAKE_CURRENT_SOURCE_DIR})
set(DST ${CMAKE_CURRENT_BINARY_DIR})

file(CREATE_LINK "${SRC}/schedy.bats" "${DST}/schedy.bats" COPY_ON_ERROR)
file(CREATE_LINK "${SRC}/pressy.bats" "${DST}/pressy.bats" COPY_ON_ERROR)
file(CREATE_LINK "${SRC}/scanny.bats" "${DST}/scanny.bats" COPY_ON_ERROR)
file(CREATE_LINK "${SRC}/helper.bash" "${DST}/helper.bash" COPY_ON_ERROR)

# include(ExternalData) set(URL https://pub.danilohorta.me)
# set(ExternalData_URL_TEMPLATES "${URL}/deciphon/%(algo)/%(hash)")

# file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/.env" "DCP_NUM_THREADS=1\n")
# file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/.env" "DCP_API_HOST=127.0.0.1\n")
# file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/.env" "DCP_API_PORT=49329\n")
# file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/.env" "DCP_API_PREFIX=\n")
# file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/.env" "DCP_API_KEY=change-me\n")
#
# find_program(CURL curl) if(CURL_NOTFOUND) message(FATAL_ERROR "curl program
# not found!") else() message(STATUS "CURL: ${CURL}") endif()
#
# add_subdirectory(tools) execute_process( COMMAND
# "${CMAKE_CURRENT_BINARY_DIR}/tools/is_api_online" ECHO_ERROR_VARIABLE TIMEOUT
# 2 COMMAND_ERROR_IS_FATAL ANY)
#
# add_executable(protein_h3reader protein_h3reader.c)
# target_link_libraries(protein_h3reader PRIVATE DECIPHON::deciphon)
# ExternalData_add_test(data_protein_h3reader NAME test_protein_h3reader COMMAND
# protein_h3reader DATA{PF02545.hmm})
# ExternalData_add_target(data_protein_h3reader)
#
# add_test(NAME test_schedy_connection COMMAND
# "${CMAKE_CURRENT_SOURCE_DIR}/schedy_connection" $<TARGET_FILE:schedy>)
#
# ExternalData_add_test( data_schedy_hmm NAME test_schedy_hmm COMMAND
# "${CMAKE_CURRENT_SOURCE_DIR}/schedy_hmm" $<TARGET_FILE:schedy>
# DATA{PF02545.hmm}) ExternalData_add_target(data_schedy_hmm)
#
# ExternalData_add_test( data_schedy_db NAME test_schedy_db COMMAND
# "${CMAKE_CURRENT_SOURCE_DIR}/schedy_db" $<TARGET_FILE:schedy>
# DATA{PF02545.hmm} DATA{PF02545.dcp}) ExternalData_add_target(data_schedy_db)
#
# ExternalData_add_test( data_schedy_job NAME test_schedy_job COMMAND
# "${CMAKE_CURRENT_SOURCE_DIR}/schedy_job" $<TARGET_FILE:schedy>
# DATA{PF02545.hmm}) ExternalData_add_target(data_schedy_job)
#
# ExternalData_add_test( data_schedy_scan NAME test_schedy_scan COMMAND
# "${CMAKE_CURRENT_SOURCE_DIR}/schedy_scan" $<TARGET_FILE:schedy>
# DATA{minifam.hmm} DATA{minifam.dcp} DATA{consensus.faa} DATA{prods_file.tsv})
# ExternalData_add_target(data_schedy_scan)
#
# # deciphon_add_test(schedy_overall)
