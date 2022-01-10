project(common)

find_package(xxHash REQUIRED)

add_library(
  ${PROJECT_NAME} STATIC
  src/bitops.c
  src/logger.c
  src/safe.c
  src/to.c
  src/xfile.c
)
add_library(DECIPHON::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PRIVATE xxHash::xxHash)

add_subdirectory(test)
