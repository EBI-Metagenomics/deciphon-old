set(TMPDIR "${CMAKE_CURRENT_BINARY_DIR}/tmp")
file(MAKE_DIRECTORY "${TMPDIR}")
add_compile_definitions(TMPDIR=\"${TMPDIR}\")

include(ExternalData)
set(ExternalData_URL_TEMPLATES
    "https://pub.danilohorta.me/deciphon/%(algo)/%(hash)")

file(COPY .env.test DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
file(COPY unittest DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

function(deciphon_add_test name)
  add_executable(${name} ${name}.c)
  target_link_libraries(${name} PRIVATE DECIPHON::deciphon)
  add_test(NAME test_${name} COMMAND ${name})
endfunction()

deciphon_add_test(standard_profile)
deciphon_add_test(protein_model)
deciphon_add_test(protein_profile)
deciphon_add_test(protein_db)

function(deciphon_add_data_test name filename)
  add_executable(${name} ${name}.c)
  target_link_libraries(${name} PRIVATE DECIPHON::deciphon)
  ExternalData_add_test(data_${name} NAME test_${name} COMMAND ${name}
                        DATA{${filename}})
  ExternalData_add_target(data_${name})
endfunction()

deciphon_add_data_test(protein_h3reader PF02545.hmm)

# sched_add_test(sched sched.c) server_add_test(server server.c)
