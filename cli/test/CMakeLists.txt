find_package(hope REQUIRED)

function(dcp_add_test name srcs)
  add_executable(${name} ${srcs})
  target_code_coverage(${name} AUTO ALL)
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

dcp_add_test(press press.c)
