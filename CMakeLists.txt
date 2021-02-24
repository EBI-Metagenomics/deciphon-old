cmake_minimum_required(VERSION 3.9 FATAL_ERROR)
project(deciphon VERSION 0.0.1 LANGUAGES C)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Generate compile_commands.json file
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
# Set RPATH only if it's not a system directory
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
if("${isSystemDir}" STREQUAL "-1")
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
endif()

# merge request fix: https://gitlab.kitware.com/cmake/cmake/-/merge_requests/1799
if(CMAKE_VERSION VERSION_GREATER 3.12.0)
    # Hide things by default
    set(CMAKE_C_VISIBILITY_PRESET hidden)
    set(CMAKE_VISIBILITY_INLINES_HIDDEN YES)
endif()

find_package(elapsed REQUIRED)
find_package(imm REQUIRED)
find_package(nmm REQUIRED)
find_package(Threads REQUIRED)
option(OPENMP "Enable OpenMP" On)
if(OPENMP)
    find_package(OpenMP)
endif()

add_library(deciphon
    src/file.c
    src/input.c
    src/master.c
    src/output.c
)
add_library(DECIPHON::deciphon ALIAS deciphon)

# Write out mytools_export.h to the current binary directory
# Must include ${CMAKE_CURRENT_BINARY_DIR} in the target's
# search path.
include(GenerateExportHeader)
generate_export_header(deciphon BASE_NAME DECIPHON
    INCLUDE_GUARD_NAME DECIPHON_EXPORT_H
    EXPORT_MACRO_NAME DCP_API
    EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/deciphon/export.h)

if(${CMAKE_C_COMPILER_ID} MATCHES "MSVC")
    set(WARNING_FLAGS /W3)
else()
    set(WARNING_FLAGS -Wall -Wextra -pedantic -Wstrict-prototypes -Wshadow
        -Wconversion -Wmissing-prototypes -Wno-unused-parameter -Wsign-conversion
        -Wno-unused-function # FIXME: added only to please clangd for now
        -Wno-empty-translation-unit # FIXME: added only to please clangd for now
    )
endif()

target_include_directories(deciphon
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(deciphon PUBLIC IMM::imm)
target_link_libraries(deciphon PUBLIC NMM::nmm)
target_link_libraries(deciphon PUBLIC ELAPSED::elapsed)
target_link_libraries(deciphon PUBLIC ck)
target_link_libraries(deciphon PRIVATE $<$<BOOL:${OpenMP_C_FOUND}>:OpenMP::OpenMP_C>)
target_link_libraries(deciphon PUBLIC Threads::Threads)
set_target_properties(deciphon PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(deciphon PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})
target_compile_features(deciphon PRIVATE c_std_11)
target_compile_definitions(deciphon PRIVATE $<$<BOOL:${WIN32}>:_CRT_NONSTDC_NO_DEPRECATE>)
target_compile_definitions(deciphon PRIVATE $<$<BOOL:${WIN32}>:__extension__>)
target_compile_options(deciphon PRIVATE ${WARNING_FLAGS})
target_compile_options(deciphon PRIVATE "-fverbose-asm")
target_link_libraries(deciphon INTERFACE $<$<BOOL:${UNIX}>:m>)

include(CheckCSourceCompiles)
check_c_source_compiles(
    "
        static void f(char const* err, ...) __attribute__((format(printf, 1, 2)));
        int main(void) {return 0;}
    "
    HAVE_ATTR_FORMAT
)
target_compile_definitions(deciphon PRIVATE $<$<BOOL:${HAVE_ATTR_FORMAT}>:HAVE_ATTR_FORMAT>)

include(CheckTypeSize)
check_type_size("intptr_t" SIZEOF_INTPTR_T)
if(NOT HAVE_SIZEOF_INTPTR_T)
    message(FATAL_ERROR "Could not determine the size of intptr_t.")
endif()
target_compile_definitions(deciphon PRIVATE SIZEOF_INTPTR_T=${SIZEOF_INTPTR_T})

install(TARGETS deciphon EXPORT deciphon-targets
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)
install(DIRECTORY include/ DESTINATION include)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/deciphon/export.h
    DESTINATION include/deciphon
)
install(EXPORT deciphon-targets
    FILE deciphon-targets.cmake
    NAMESPACE DECIPHON::
    DESTINATION lib/cmake/deciphon
)
include(CMakePackageConfigHelpers)
configure_package_config_file(deciphon-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/deciphon-config.cmake
    INSTALL_DESTINATION lib/cmake/deciphon
    NO_SET_AND_CHECK_MACRO
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/deciphon-config.cmake
    DESTINATION lib/cmake/deciphon
)

enable_testing()
add_subdirectory(test)

option(COVERAGE "Enable coverage")
if(COVERAGE)
    string(TOUPPER "${CMAKE_BUILD_TYPE}" uppercase_CMAKE_BUILD_TYPE)
    if(NOT ${uppercase_CMAKE_BUILD_TYPE} MATCHES "DEBUG")
        message(FATAL_ERROR "CMAKE_BUILD_TYPE must be set to Debug.")
    endif()

    include(CodeCoverage)
    append_coverage_compiler_flags()
    setup_target_for_coverage_lcov(
        NAME coverage
        EXECUTABLE ctest
        DEPENDENCIES all_tests)
endif()
