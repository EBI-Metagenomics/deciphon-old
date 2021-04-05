add_custom_target(all_tests)
find_package(cass REQUIRED)

function(dcp_add_test name srcs)
    add_executable(test_${name} ${srcs})
    add_dependencies(all_tests test_${name})
    target_link_libraries(test_${name} PRIVATE DECIPHON::dcp)
    target_link_libraries(test_${name} PUBLIC CASS::cass)
    target_compile_options(test_${name} PRIVATE ${WARNING_FLAGS})
    target_compile_features(test_${name} PRIVATE c_std_11)
    add_test(NAME ${name} COMMAND test_${name})

    set(TMPDIR "${CMAKE_CURRENT_BINARY_DIR}/test_${name}.tmp")
    add_custom_command(TARGET test_${name} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${TMPDIR}
    )
    target_compile_definitions(test_${name} PUBLIC "TMPDIR=\"${TMPDIR}\"")
endfunction()

# dcp_add_test(io)
# dcp_add_test(pfam24)
dcp_add_test(server server.c)
dcp_add_test(metadata metadata.c)
dcp_add_test(tasks "tasks.c;pfam24_results.c")
