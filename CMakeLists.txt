cmake_minimum_required(VERSION 3.9 FATAL_ERROR)
project(deciphon VERSION 0.0.1 LANGUAGES C)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

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

add_library(dcp
    src/gc.c
    src/input.c
    src/metadata.c
    src/mpool.c
    src/output.c
    src/profile.c
    src/result.c
    src/result_ring.c
    src/results.c
    src/results_queue.c
    src/scan.c
    src/seq_stack.c
    src/server.c
    src/special_trans.c
    src/str.c
    src/task.c
    src/task_bin.c
    src/task_queue.c
    src/util.c
)
add_library(DECIPHON::dcp ALIAS dcp)

# Write out mytools_export.h to the current binary directory
# Must include ${CMAKE_CURRENT_BINARY_DIR} in the target's
# search path.
include(GenerateExportHeader)
generate_export_header(dcp BASE_NAME DCP
    INCLUDE_GUARD_NAME DCP_EXPORT_H
    EXPORT_MACRO_NAME DCP_API
    EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/dcp/export.h)

if(${CMAKE_C_COMPILER_ID} MATCHES "MSVC")
    set(WARNING_FLAGS /W3)
else()
    set(WARNING_FLAGS -Wall -Wextra -pedantic -Wstrict-prototypes -Wshadow
        -Wconversion -Wmissing-prototypes -Wno-unused-parameter -Wsign-conversion
        -Wno-gnu-statement-expression
        -Wno-unused-function # FIXME: added only to please clangd for now
        -Wno-empty-translation-unit # FIXME: added only to please clangd for now
    )
endif()

target_include_directories(dcp
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(dcp PUBLIC IMM::imm)
target_link_libraries(dcp PUBLIC NMM::nmm)
target_link_libraries(dcp PUBLIC ELAPSED::elapsed)
target_link_libraries(dcp PRIVATE Threads::Threads)
target_link_libraries(dcp PUBLIC ck)
target_link_libraries(dcp PRIVATE $<$<BOOL:${OpenMP_C_FOUND}>:OpenMP::OpenMP_C>)
set_target_properties(dcp PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(dcp PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})
target_compile_definitions(dcp PRIVATE $<$<BOOL:${WIN32}>:_CRT_NONSTDC_NO_DEPRECATE>)
target_compile_definitions(dcp PRIVATE $<$<BOOL:${WIN32}>:__extension__>)
target_compile_options(dcp PRIVATE ${WARNING_FLAGS})
target_compile_options(dcp PRIVATE "-fverbose-asm")
target_link_libraries(dcp INTERFACE $<$<BOOL:${UNIX}>:m>)

include(CheckCSourceCompiles)
check_c_source_compiles(
    "
        static void f(char const* err, ...) __attribute__((format(printf, 1, 2)));
        int main(void) {return 0;}
    "
    HAVE_ATTR_FORMAT
)
target_compile_definitions(dcp PRIVATE $<$<BOOL:${HAVE_ATTR_FORMAT}>:HAVE_ATTR_FORMAT>)

include(CheckTypeSize)
check_type_size("intptr_t" SIZEOF_INTPTR_T)
if(NOT HAVE_SIZEOF_INTPTR_T)
    message(FATAL_ERROR "Could not determine the size of intptr_t.")
endif()
target_compile_definitions(dcp PRIVATE SIZEOF_INTPTR_T=${SIZEOF_INTPTR_T})

install(TARGETS dcp EXPORT dcp-targets
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)
install(DIRECTORY include/ DESTINATION include)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/dcp/export.h
    DESTINATION include/dcp
)
install(EXPORT dcp-targets
    FILE dcp-targets.cmake
    NAMESPACE DECIPHON::
    DESTINATION lib/cmake/dcp
)
include(CMakePackageConfigHelpers)
configure_package_config_file(dcp-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/dcp-config.cmake
    INSTALL_DESTINATION lib/cmake/dcp
    NO_SET_AND_CHECK_MACRO
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dcp-config.cmake
    DESTINATION lib/cmake/dcp
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
