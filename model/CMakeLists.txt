add_library(
  model
  nuclt_dist.c
  profile.c
  profile_types.c
  protein_codec.c
  protein_h3reader.c
  protein_model.c
  protein_profile.c
  protein_state.c
  standard_profile.c
  standard_state.c)

target_link_libraries(model PUBLIC DECIPHON::core)
target_link_libraries(model PUBLIC HMR::hmr)
target_link_libraries(model PUBLIC IMM::imm)
target_link_libraries(model PUBLIC LITE_PACK::lite_pack)
target_link_libraries(model PUBLIC C_TOOLBELT::c_toolbelt)
