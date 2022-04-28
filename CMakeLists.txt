cmake_minimum_required(VERSION 3.16.3 FATAL_ERROR)

project(
  deciphon
  VERSION 1.0.0
  LANGUAGES C)

include(CheckSymbolExists)
include(cmake/compiler-options.cmake)
include(cmake/sanitizers.cmake)
include(cmake/CPM.cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

cpmaddpackage(NAME hmr GIT_TAG 0.1.2 GITHUB_REPOSITORY
              EBI-Metagenomics/hmmer-reader)
cpmaddpackage(NAME imm GIT_TAG 0.1.1 GITHUB_REPOSITORY EBI-Metagenomics/imm)
cpmaddpackage(NAME c_toolbelt GIT_TAG 0.1.2 GITHUB_REPOSITORY horta/c-toolbelt)
cpmaddpackage(NAME lite_pack GIT_TAG 0.1.2 GITHUB_REPOSITORY
              EBI-Metagenomics/lite-pack)

find_package(CURL REQUIRED)
find_package(Threads REQUIRED)
find_package(hmr REQUIRED)
find_package(imm REQUIRED)
find_package(lite_pack REQUIRED)
find_package(c_toolbelt REQUIRED)
find_package(Argp REQUIRED)
find_package(OpenMP REQUIRED)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)

option(DEBUG_HTTP_REQUESTS "Debug HTTP requests" OFF)
if(DEBUG_HTTP_REQUESTS)
  message(STATUS "Debug HTTP requests enabled")
else()
  message(STATUS "Debug HTTP requests disabled")
  add_compile_definitions(NDEBUG_HTTP_REQUESTS=1)
endif()

add_compile_definitions(_XOPEN_SOURCE=700)
add_compile_definitions(_DARWIN_C_SOURCE=1)

set(LIBS core model db sched server cli)

# Enable interprodecural optimization
macro(enable_ipo lib)
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    include(CheckIPOSupported)
    check_ipo_supported()
    set_target_properties(${lib} PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
  endif()
endmacro()

foreach(lib ${LIBS})
  add_subdirectory(${lib})
  add_library(DECIPHON::${lib} ALIAS ${lib})
  enable_ipo(${lib})

  target_include_directories(
    ${lib}
    PUBLIC $<INSTALL_INTERFACE:include>
           $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
           $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
    PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/${lib}>)
endforeach()

enable_testing()
add_subdirectory(test)

set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VENDOR "Danilo Horta")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "DECIPHON")
set(CPACK_PACKAGE_CONTACT "Danilo Horta")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md)
set(CPACK_RESOURCE_FILE_README ${CMAKE_CURRENT_SOURCE_DIR}/README.md)
set(CPACK_OUTPUT_FILE_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/package)
set(CPACK_VERBATIM_VARIABLES YES)
set(CPACK_PACKAGE_RELOCATABLE YES)
set(CPACK_MONOLITHIC_INSTALL YES)
include(CPack)
