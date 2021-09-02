add_custom_target(all_tests)
find_package(hope REQUIRED)

set(ASSETS ${CMAKE_CURRENT_BINARY_DIR}/assets)
file(MAKE_DIRECTORY ${ASSETS})
set(URL "https://deciphon.s3.eu-west-2.amazonaws.com")

file(DOWNLOAD ${URL}/PF02545.hmm.7z ${ASSETS}/PF02545.hmm.7z
     EXPECTED_HASH SHA1=2b27ec36086e5c424b719437a924728801bb8277)
file(ARCHIVE_EXTRACT INPUT ${ASSETS}/PF02545.hmm.7z DESTINATION ${ASSETS})

# include(FetchContent) FetchContent_Declare( pfam24 URL
# https://deciphon.s3.eu-west-2.amazonaws.com/pfam24.dcp.7z URL_HASH
# SHA1=1d52d936292c3b572efe73ef6830476aa416ced9)

# FetchContent_MakeAvailable(pfam24) FetchContent_GetProperties(pfam24
# SOURCE_DIR srcdir) set(PFAM24_FILEPATH ${srcdir}/pfam24.dcp)

function(dcp_add_test name srcs)
  add_executable(${name} ${srcs})
  add_dependencies(all_tests ${name})
  target_link_libraries(${name} PRIVATE DCP::dcp)
  target_link_libraries(${name} PRIVATE HOPE::hope)
  target_compile_options(${name} PRIVATE ${WARNING_FLAGS})
  target_compile_features(${name} PRIVATE c_std_11)
  add_test(NAME ${name} COMMAND ${name})

  set(TMPDIR "${CMAKE_CURRENT_BINARY_DIR}/${name}.tmp")
  add_custom_command(
    TARGET ${name}
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${TMPDIR})
  target_compile_definitions(${name} PUBLIC "ASSETS=\"${ASSETS}\"")
  target_compile_definitions(${name} PUBLIC "TMPDIR=\"${TMPDIR}\"")
endfunction()

dcp_add_test(std_profile std_profile.c)
dcp_add_test(std_db std_db.c)

dcp_add_test(pro_model pro_model.c)
dcp_add_test(pro_profile pro_profile.c)
dcp_add_test(pro_db pro_db.c)
dcp_add_test(pro_reader pro_reader.c)

# dcp_add_test(server server.c) dcp_add_test(pp "pp.c;data.c")
# dcp_add_test(db_pp "db_pp.c;data.c") dcp_add_test(metadata metadata.c)
# dcp_add_test(tasks "tasks.c;pfam24_results.c") dcp_add_test(very_long_seq
# "very_long_seq.c")
