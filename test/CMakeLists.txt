include(helper.cmake)

set(tmpdir "${CMAKE_CURRENT_BINARY_DIR}/tmp")
set(assets "${CMAKE_CURRENT_BINARY_DIR}/assets")
file(MAKE_DIRECTORY "${tmpdir}")
file(MAKE_DIRECTORY "${assets}")

add_compile_definitions(TMPDIR="${tmpdir}")
add_compile_definitions(ASSETS="${assets}")

deciphon_download(minifam.hmm)
deciphon_download(minifam.dcp)
deciphon_download(minifam.hmm)
deciphon_download(consensus.json)
deciphon_download(prods_file_20221021.tsv)

deciphon_add_test(standard_profile DECIPHON::deciphon)
deciphon_add_test(protein_model DECIPHON::deciphon)
deciphon_add_test(protein_profile DECIPHON::deciphon)
deciphon_add_test(protein_db DECIPHON::deciphon)
deciphon_add_test(scan DECIPHON::deciphon DECIPHON::scanny_static)

set(SRC ${CMAKE_CURRENT_SOURCE_DIR})
set(DST ${CMAKE_CURRENT_BINARY_DIR})

# file(CREATE_LINK "${SRC}/decy.bats" "${DST}/decy.bats" COPY_ON_ERROR)
file(CREATE_LINK "${SRC}/schedy.bats" "${DST}/schedy.bats" COPY_ON_ERROR)
file(CREATE_LINK "${SRC}/pressy.bats" "${DST}/pressy.bats" COPY_ON_ERROR)
file(CREATE_LINK "${SRC}/scanny.bats" "${DST}/scanny.bats" COPY_ON_ERROR)
file(CREATE_LINK "${SRC}/helper.bash" "${DST}/helper.bash" COPY_ON_ERROR)
file(CREATE_LINK "${SRC}/manifest" "${DST}/manifest" COPY_ON_ERROR)

find_program(BATS_PROGRAM bats REQUIRED)

add_test(test_pressy "${BATS_PROGRAM}" "${DST}/pressy.bats")
add_test(test_scanny "${BATS_PROGRAM}" "${DST}/scanny.bats")
add_test(test_schedy "${BATS_PROGRAM}" "${DST}/schedy.bats")
# add_test(test_decy "${BATS_PROGRAM}" "${DST}/decy.bats")
