add_executable(dotenv_get dotenv_get.c)
target_link_libraries(dotenv_get PRIVATE DOTENV::dotenv)
target_compile_features(dotenv_get PRIVATE c_std_11)
target_compile_options(dotenv_get PRIVATE ${WARNING_FLAGS})

file(COPY unittest DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
file(COPY abspath DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
