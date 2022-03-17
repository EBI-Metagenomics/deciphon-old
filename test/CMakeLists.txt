find_package(hope REQUIRED)

set(ASSETS ${CMAKE_CURRENT_BINARY_DIR}/assets)
set(TMPDIR ${CMAKE_CURRENT_BINARY_DIR}/tmp)
set(REST_URL_STEM "http://127.0.0.1:8000/api")

file(MAKE_DIRECTORY ${ASSETS})
file(MAKE_DIRECTORY ${TMPDIR})

add_compile_definitions(ASSETS=\"${ASSETS}\")
add_compile_definitions(TMPDIR=\"${TMPDIR}\")
add_compile_definitions(REST_URL_STEM=\"${REST_URL_STEM}\")

set(URL "https://deciphon.s3.eu-west-2.amazonaws.com")

file(DOWNLOAD ${URL}/PF02545.hmm.7z ${ASSETS}/PF02545.hmm.7z
     EXPECTED_HASH SHA1=2b27ec36086e5c424b719437a924728801bb8277)
file(ARCHIVE_EXTRACT INPUT ${ASSETS}/PF02545.hmm.7z DESTINATION ${ASSETS})
#
# file( DOWNLOAD ${URL}/Pfam-A.5.hmm.7z ${ASSETS}/Pfam-A.5.hmm.7z EXPECTED_HASH
# SHA1=1dcdd145730f2a9e2314132972775ec284fb02e1 ) file(ARCHIVE_EXTRACT INPUT
# ${ASSETS}/Pfam-A.5.hmm.7z DESTINATION ${ASSETS})
#
# file( DOWNLOAD ${URL}/pfam24.dcp.7z ${ASSETS}/pfam24.dcp.7z EXPECTED_HASH
# SHA1=a4c23440c1f69fb7d0d64b8f29781e06969e83bd ) file(ARCHIVE_EXTRACT INPUT
# ${ASSETS}/pfam24.dcp.7z DESTINATION ${ASSETS})

function(model_add_test name libs)
  add_executable(test_${name} ${name}.c)
  target_link_libraries(test_${name} PRIVATE HOPE::hope)
  target_link_libraries(test_${name} PRIVATE DECIPHON::core)
  target_link_libraries(test_${name} PRIVATE DECIPHON::model)
  add_test(NAME test_${name} COMMAND test_${name})
endfunction()

function(db_add_test name libs)
  add_executable(test_${name} ${name}.c)
  target_link_libraries(test_${name} PRIVATE HOPE::hope)
  target_link_libraries(test_${name} PRIVATE DECIPHON::core)
  target_link_libraries(test_${name} PRIVATE DECIPHON::model)
  target_link_libraries(test_${name} PRIVATE DECIPHON::db)
  add_test(NAME test_${name} COMMAND test_${name})
endfunction()

function(server_add_test name libs)
  add_executable(test_${name} ${name}.c)
  target_link_libraries(test_${name} PRIVATE HOPE::hope)
  target_link_libraries(test_${name} PRIVATE DECIPHON::core)
  target_link_libraries(test_${name} PRIVATE DECIPHON::model)
  target_link_libraries(test_${name} PRIVATE DECIPHON::db)
  target_link_libraries(test_${name} PRIVATE DECIPHON::server)
  add_test(NAME test_${name} COMMAND test_${name})
endfunction()

model_add_test(standard_profile standard_profile.c)
model_add_test(protein_model protein_model.c)
model_add_test(protein_profile protein_profile.c)
model_add_test(protein_h3reader protein_h3reader.c)

db_add_test(protein_db protein_db.c)

server_add_test(rest rest.c)
server_add_test(server server.c)
