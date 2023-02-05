include(deciphon.cmake)

set(TMPDIR "${CMAKE_CURRENT_BINARY_DIR}/tmp")
set(ASSETS "${CMAKE_CURRENT_BINARY_DIR}/assets")

file(MAKE_DIRECTORY "${TMPDIR}")
file(MAKE_DIRECTORY "${ASSETS}")

add_compile_definitions(TMPDIR="${TMPDIR}")
add_compile_definitions(ASSETS="${ASSETS}")
dcp_download_assets("${ASSETS}")

dcp_add_test(model)
dcp_add_test(protein)
dcp_add_test(db)
dcp_add_test(press)
dcp_add_test(scan)

add_test(NAME start_hmmer COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/start_hmmer.sh ${ASSETS}/minifam.hmm)
add_test(NAME stop_hmmer COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/stop_hmmer.sh ${ASSETS}/minifam.hmm)

set_tests_properties(scan PROPERTIES FIXTURES_REQUIRED hmmer)
set_tests_properties(start_hmmer PROPERTIES FIXTURES_SETUP hmmer)
set_tests_properties(stop_hmmer PROPERTIES FIXTURES_CLEANUP hmmer)
