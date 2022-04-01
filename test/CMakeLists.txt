find_package(hope REQUIRED)

set(ASSETS ${CMAKE_CURRENT_BINARY_DIR}/assets)
set(TMPDIR ${CMAKE_CURRENT_BINARY_DIR}/tmp)
set(SCHED_API_URL
    "http://127.0.0.1:8000/api"
    CACHE STRING "Scheduler API URL")
message(STATUS "Scheduler API URL: ${SCHED_API_URL}")

file(MAKE_DIRECTORY ${ASSETS})
file(MAKE_DIRECTORY ${TMPDIR})

add_compile_definitions(ASSETS=\"${ASSETS}\")
add_compile_definitions(TMPDIR=\"${TMPDIR}\")
add_compile_definitions(SCHED_API_URL=\"${SCHED_API_URL}\")

set(URL "https://f002.backblazeb2.com/file/deciphon")

file(DOWNLOAD ${URL}/PF02545.hmm.7z ${ASSETS}/PF02545.hmm.7z
     EXPECTED_HASH SHA1=2b27ec36086e5c424b719437a924728801bb8277)
file(ARCHIVE_EXTRACT INPUT ${ASSETS}/PF02545.hmm.7z DESTINATION ${ASSETS})

file(DOWNLOAD ${URL}/PF02545.dcp.7z ${ASSETS}/PF02545.dcp.7z
     EXPECTED_HASH SHA1=cce596ce9695b9d1e862e300b86df840adccbc14)
file(ARCHIVE_EXTRACT INPUT ${ASSETS}/PF02545.dcp.7z DESTINATION ${ASSETS})

file(DOWNLOAD ${URL}/Pfam-A.one_tenth.hmm.7z ${ASSETS}/Pfam-A.one_tenth.hmm.7z
     EXPECTED_HASH MD5=8203bd09489c5b097c7e3054b96f57e0)
file(ARCHIVE_EXTRACT INPUT ${ASSETS}/Pfam-A.one_tenth.hmm.7z DESTINATION
     ${ASSETS})

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

function(sched_add_test name libs)
  add_executable(test_${name} ${name}.c)
  target_link_libraries(test_${name} PRIVATE HOPE::hope)
  target_link_libraries(test_${name} PRIVATE DECIPHON::core)
  target_link_libraries(test_${name} PRIVATE DECIPHON::model)
  target_link_libraries(test_${name} PRIVATE DECIPHON::db)
  target_link_libraries(test_${name} PRIVATE DECIPHON::sched)
  add_test(NAME test_${name} COMMAND test_${name})
endfunction()

function(server_add_test name libs)
  add_executable(test_${name} ${name}.c)
  target_link_libraries(test_${name} PRIVATE HOPE::hope)
  target_link_libraries(test_${name} PRIVATE DECIPHON::core)
  target_link_libraries(test_${name} PRIVATE DECIPHON::model)
  target_link_libraries(test_${name} PRIVATE DECIPHON::db)
  target_link_libraries(test_${name} PRIVATE DECIPHON::sched)
  target_link_libraries(test_${name} PRIVATE DECIPHON::server)
  add_test(NAME test_${name} COMMAND test_${name})
endfunction()

model_add_test(standard_profile standard_profile.c)
model_add_test(protein_model protein_model.c)
model_add_test(protein_profile protein_profile.c)
model_add_test(protein_h3reader protein_h3reader.c)

db_add_test(protein_db protein_db.c)

sched_add_test(sched sched.c)

server_add_test(server server.c)
