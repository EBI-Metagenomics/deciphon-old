find_package(hope REQUIRED)

set(ASSETS ${CMAKE_CURRENT_BINARY_DIR}/assets)
file(MAKE_DIRECTORY ${ASSETS})
set(URL "https://deciphon.s3.eu-west-2.amazonaws.com")

file(
  DOWNLOAD ${URL}/PF02545.hmm.7z ${ASSETS}/PF02545.hmm.7z
  EXPECTED_HASH SHA1=2b27ec36086e5c424b719437a924728801bb8277
)
file(ARCHIVE_EXTRACT INPUT ${ASSETS}/PF02545.hmm.7z DESTINATION ${ASSETS})

file(
  DOWNLOAD ${URL}/Pfam-A.5.hmm.7z ${ASSETS}/Pfam-A.5.hmm.7z
  EXPECTED_HASH SHA1=1dcdd145730f2a9e2314132972775ec284fb02e1
)
file(ARCHIVE_EXTRACT INPUT ${ASSETS}/Pfam-A.5.hmm.7z DESTINATION ${ASSETS})

function(dcp_add_test name srcs)
  add_executable(${name} ${srcs})
  target_link_libraries(${name} PRIVATE DCP::dcp)
  target_link_libraries(${name} PRIVATE HOPE::hope)
  target_compile_options(${name} PRIVATE ${WARNING_FLAGS})
  target_compile_features(${name} PRIVATE c_std_11)
  add_test(NAME ${name} COMMAND ${name})

  set(TMPDIR "${CMAKE_CURRENT_BINARY_DIR}/${name}.tmp")
  add_custom_command(
    TARGET ${name}
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${TMPDIR}
  )
  target_compile_definitions(${name} PUBLIC "ASSETS=\"${ASSETS}\"")
  target_compile_definitions(${name} PUBLIC "TMPDIR=\"${TMPDIR}\"")
endfunction()

dcp_add_test(standard_profile standard_profile.c)
dcp_add_test(standard_db "standard_db.c;standard_db_examples.c")

dcp_add_test(protein_model protein_model.c)
dcp_add_test(protein_profile protein_profile.c)
dcp_add_test(protein_db "protein_db.c;protein_db_examples.c")
dcp_add_test(protein_reader protein_reader.c)
# dcp_add_test(srv "srv.c;standard_db_examples.c;pro_db_examples.c;srv_data.c")
# dcp_add_test(job job.c)

# CLI
# dcp_add_test(cli_press cli_press.c)

# dcp_add_test(db_pp "db_pp.c;data.c") dcp_add_test(metadata metadata.c)
# dcp_add_test(tasks "tasks.c;pfam24_results.c") dcp_add_test(very_long_seq
# "very_long_seq.c")
