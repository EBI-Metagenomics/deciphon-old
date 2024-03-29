set(ASSETS ${CMAKE_CURRENT_BINARY_DIR}/assets)
set(TMPDIR ${CMAKE_CURRENT_BINARY_DIR}/tmp)

file(MAKE_DIRECTORY ${ASSETS})
file(MAKE_DIRECTORY ${TMPDIR})

add_compile_definitions(ASSETS=\"${ASSETS}\")
add_compile_definitions(TMPDIR=\"${TMPDIR}\")

function(download_files dst)
  set(URL "https://uk1s3.embassy.ebi.ac.uk/deciphon")

  file(COPY assets/.env.test DESTINATION ${dst})

  file(DOWNLOAD ${URL}/PF02545.hmm.7z ${dst}/PF02545.hmm.7z
       EXPECTED_HASH MD5=0e1ebe82f2b78b4ed56250b8661e9d0d)
  file(ARCHIVE_EXTRACT INPUT ${dst}/PF02545.hmm.7z DESTINATION ${dst})

  file(DOWNLOAD ${URL}/PF02545.dcp.7z ${dst}/PF02545.dcp.7z
       EXPECTED_HASH MD5=310a817bf6fe6096dfa228e637a96515)
  file(ARCHIVE_EXTRACT INPUT ${dst}/PF02545.dcp.7z DESTINATION ${dst})

  file(DOWNLOAD ${URL}/Pfam-A.one_tenth.hmm.7z ${dst}/Pfam-A.one_tenth.hmm.7z
       EXPECTED_HASH MD5=8203bd09489c5b097c7e3054b96f57e0)
  file(ARCHIVE_EXTRACT INPUT ${dst}/Pfam-A.one_tenth.hmm.7z DESTINATION ${dst})
endfunction()

download_files("${ASSETS}")

function(model_add_test name libs)
  add_executable(test_${name} ${name}.c)
  target_link_libraries(test_${name} PRIVATE DECIPHON::deciphon)
  add_test(NAME test_${name} COMMAND test_${name})
endfunction()

function(db_add_test name libs)
  add_executable(test_${name} ${name}.c)
  target_link_libraries(test_${name} PRIVATE DECIPHON::deciphon)
  add_test(NAME test_${name} COMMAND test_${name})
endfunction()

function(sched_add_test name libs)
  add_executable(test_${name} ${name}.c)
  target_link_libraries(test_${name} PRIVATE DECIPHON::deciphon)
  add_test(NAME test_${name} COMMAND test_${name})
endfunction()

function(server_add_test name libs)
  add_executable(test_${name} ${name}.c)
  target_link_libraries(test_${name} PRIVATE DECIPHON::deciphon)
  add_test(NAME test_${name} COMMAND test_${name})
endfunction()

model_add_test(standard_profile standard_profile.c)
model_add_test(protein_model protein_model.c)
model_add_test(protein_profile protein_profile.c)
model_add_test(protein_h3reader protein_h3reader.c)

db_add_test(protein_db protein_db.c)

# sched_add_test(sched sched.c) server_add_test(server server.c)
