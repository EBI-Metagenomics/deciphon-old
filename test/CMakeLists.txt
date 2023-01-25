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
