add_library(
  deciphon_press
  press.c)
add_library(DECIPHON::deciphon_press ALIAS deciphon_press)

target_link_libraries(deciphon_press PUBLIC deciphon)
target_compile_options(deciphon_press PRIVATE ${WARNING_FLAGS})
target_include_directories(deciphon_press PRIVATE ${CMAKE_SOURCE_DIR})
