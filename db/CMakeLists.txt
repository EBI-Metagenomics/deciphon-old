add_library(db reader.c writer.c protein_writer.c protein_reader.c
               profile_reader.c)

target_link_libraries(db PUBLIC C_TOOLBELT::c_toolbelt)
target_link_libraries(db PUBLIC LITE_PACK::lite_pack)
target_link_libraries(db PUBLIC IMM::imm)
target_link_libraries(db PUBLIC HMR::hmr)
