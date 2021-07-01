add_custom_target(all_fuzzs)

function(dcp_add_fuzz tgt seed max_len runs)
    add_executable(${tgt} ${tgt}.c)
    target_link_libraries(${tgt} PRIVATE DCP::dcp)
    target_compile_options(${tgt} PRIVATE ${WARNING_FLAGS})
    target_compile_features(${tgt} PRIVATE c_std_11)

    add_executable(${seed} ${seed}.c)
    target_link_libraries(${seed} PRIVATE DCP::dcp)
    target_compile_options(${seed} PRIVATE ${WARNING_FLAGS})
    target_compile_features(${seed} PRIVATE c_std_11)

    add_dependencies(all_fuzzs ${tgt} ${seed})

    set(CORPUS "${CMAKE_CURRENT_BINARY_DIR}/${tgt}.corpus")
    target_compile_definitions(${tgt} PUBLIC "CORPUS=\"${CORPUS}\"")
    target_compile_definitions(${seed} PUBLIC "CORPUS=\"${CORPUS}\"")

    add_custom_command(
        TARGET ${seed} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CORPUS}
    )

    add_custom_command(
        TARGET ${tgt} PRE_BUILD
        COMMAND ${seed}
    )

    add_custom_command(
        TARGET ${tgt} POST_BUILD
        COMMAND ${tgt} ${CORPUS}
            -max_len=${max_len} -runs=${runs}
            -seed=1 -verbosity=0
        VERBATIM
    )
endfunction()

dcp_add_fuzz(
    db_fuzz db_seed 176160768 10000
)
