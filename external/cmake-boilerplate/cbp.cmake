set(CBP_DIR "${CMAKE_CURRENT_LIST_DIR}")
list(APPEND CMAKE_MODULE_PATH "${CBP_DIR}/cmake")

function(cbp_install tgt hdr_place)
  install(TARGETS ${tgt} EXPORT ${tgt}-targets)

  if(hdr_place STREQUAL "SEPARATE")
    install(DIRECTORY include/ DESTINATION include)
  elseif(hdr_place STREQUAL "MERGED")
    install(DIRECTORY src/${tgt}/ DESTINATION include/${tgt})
  else()
    message(FATAL_ERROR "Wrong header placement.")
  endif()

  string(TOUPPER ${tgt} TGT)
  set(dst lib/cmake/${tgt})
  install(
    EXPORT ${tgt}-targets
    FILE ${tgt}-targets.cmake
    NAMESPACE ${TGT}::
    DESTINATION ${dst}
  )

  include(CMakePackageConfigHelpers)

  set(cfg ${CMAKE_CURRENT_BINARY_DIR}/${tgt}-config.cmake)
  configure_package_config_file(
    ${tgt}-config.cmake.in ${cfg}
    INSTALL_DESTINATION ${dst}
  )

  set(ver ${CMAKE_CURRENT_BINARY_DIR}/${tgt}-config-version.cmake)
  write_basic_package_version_file(${ver} COMPATIBILITY SameMajorVersion)

  install(FILES ${cfg} ${ver} DESTINATION ${dst})
endfunction()

macro(cbp_include_sanitizers)
  include("${CBP_DIR}/cmake-scripts/sanitizers.cmake")
endmacro()

macro(cbp_include_code_coverage)
  include("${CBP_DIR}/cmake-scripts/code-coverage.cmake")
endmacro()

macro(cbp_set_warning_flags)
  if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    # /wd5105:
    # https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/c5105?view=msvc-160
    set(WARNING_FLAGS /W3 /wd5105)
  else()
    set(
      WARNING_FLAGS
      -Wall
      -Wextra
      -Wstrict-prototypes
      -Wshadow
      -Wconversion
      -Wmissing-prototypes
      -Wno-unused-parameter
      -Wsign-conversion
      -Wno-unused-function
      -Wmissing-field-initializers
    )

    if(NOT CMAKE_C_COMPILER_ID STREQUAL "GNU")
      list(
        APPEND WARNING_FLAGS -Wno-gnu-designator -Wno-empty-translation-unit
        -Wno-gnu-statement-expression -Wno-nullability-extension
        -Wconditional-uninitialized -Wgnu-empty-initializer
      )
    endif()
  endif()
endmacro()

macro(cbp_ci_build_option)
  option(CI_BUILD "CI, extra flags will be set" OFF)
  if(CI_BUILD)
    message(STATUS "CI build enabled")
    if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
      add_compile_options(/WX)
    else()
      add_compile_options(-Werror)
    endif()
  endif()
endmacro()

macro(cbp_set_rpath)
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
  # Set RPATH only if it's not a system directory
  list(
    FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES
    "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir
  )
  if("${isSystemDir}" STREQUAL "-1")
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
  endif()
endmacro()

function(cbp_hide_functions tgt)
  # merge request fix:
  # https://gitlab.kitware.com/cmake/cmake/-/merge_requests/1799
  if(CMAKE_VERSION VERSION_GREATER 3.12.0)
    # Hide functions by default.
    set_property(TARGET ${tgt} PROPERTY C_VISIBILITY_PRESET hidden)
    set_property(TARGET ${tgt} PROPERTY VISIBILITY_INLINES_HIDDEN ON)
  endif()
endfunction()

macro(cbp_generate_export_header tgt base export_file)
  set(${export_file} ${CMAKE_CURRENT_BINARY_DIR}/${tgt}/export.h)
  string(TOUPPER ${tgt} TGT)
  include(GenerateExportHeader)
  generate_export_header(
    ${tgt}
    BASE_NAME
    ${base}
    INCLUDE_GUARD_NAME
    ${TGT}_EXPORT_H
    EXPORT_MACRO_NAME
    ${base}_API
    EXPORT_FILE_NAME
    ${${export_file}}
  )
endmacro()

macro(cbp_assert_null_is_zero_bits)
  include(CheckCSourceRuns)
  check_c_source_runs(
    "
  #include <stddef.h>
  #include <stdint.h>

  /* Returns zero if NULL is zero-bits represented. */
  int main(void)
  {
    void *ptr = NULL;
    intptr_t v0 = (intptr_t)ptr;
    intptr_t v1 = 0;
    return !(v0 == v1);
  }
  "
    NULL_IS_ZERO_BITS
  )

  if(NULL_IS_ZERO_BITS)
    message(STATUS "NULL is zero-bits represented.")
  else()
    message(FATAL_ERROR "NULL is not zero-bits represented.")
  endif()
endmacro()

# Author: https://cristianadam.eu/20190501/bundling-together-static-libraries-with-cmake/
function(cbp_bundle_static_library tgt_name bundled_tgt_name)
  list(APPEND static_libs ${tgt_name})

  function(_recursively_collect_dependencies input_target)
    set(_input_link_libraries LINK_LIBRARIES)
    get_target_property(_input_type ${input_target} TYPE)
    if(${_input_type} STREQUAL "INTERFACE_LIBRARY")
      set(_input_link_libraries INTERFACE_LINK_LIBRARIES)
    endif()
    get_target_property(public_dependencies ${input_target} ${_input_link_libraries})
    message(STATUS "public_dependencies: ${public_dependencies}")
    foreach(dependency IN LISTS public_dependencies)
        message(STATUS "dependency: ${dependency}")
      if(TARGET ${dependency})
          message(STATUS "IT IS TARGET")
        get_target_property(alias ${dependency} ALIASED_TARGET)
        message(STATUS "alias: ${alias}")
        if(TARGET ${alias})
          set(dependency ${alias})
        endif()
        message(STATUS "dependency: ${dependency}")
        get_target_property(_type ${dependency} TYPE)
        message(STATUS "_type: ${_type}")
        if(${_type} STREQUAL "STATIC_LIBRARY")
          list(APPEND static_libs ${dependency})
        endif()

        get_property(
          library_already_added
          GLOBAL PROPERTY _${tgt_name}_static_bundle_${dependency}
        )
        if(NOT library_already_added)
          set_property(GLOBAL PROPERTY _${tgt_name}_static_bundle_${dependency} ON)
          _recursively_collect_dependencies(${dependency})
        endif()
      endif()
    endforeach()
    set(static_libs ${static_libs} PARENT_SCOPE)
  endfunction()

  _recursively_collect_dependencies(${tgt_name})

  list(REMOVE_DUPLICATES static_libs)

  set(
    bundled_tgt_full_name
    ${CMAKE_BINARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}${bundled_tgt_name}${CMAKE_STATIC_LIBRARY_SUFFIX}
  )

  if(CMAKE_C_COMPILER_ID MATCHES "^(Clang|GNU)$")
    file(
      WRITE ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar.in
      "CREATE ${bundled_tgt_full_name}\n"
    )

    foreach(tgt IN LISTS static_libs)
      file(
        APPEND ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar.in
        "ADDLIB $<TARGET_FILE:${tgt}>\n"
      )
    endforeach()

    file(APPEND ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar.in "SAVE\n")
    file(APPEND ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar.in "END\n")

    file(
      GENERATE
      OUTPUT ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar
      INPUT ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar.in
    )

    set(ar_tool ${CMAKE_AR})
    if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
      set(ar_tool ${CMAKE_C_COMPILER_AR})
    endif()

    add_custom_command(
      COMMAND ${ar_tool} -M < ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar
      OUTPUT ${bundled_tgt_full_name}
      COMMENT "Bundling ${bundled_tgt_name}"
      VERBATIM
    )
  elseif(CMAKE_C_COMPILER_ID MATCHES "^(AppleClang)$")

    list(APPEND args ${bundled_tgt_full_name})

    foreach(tgt IN LISTS static_libs)
      list(APPEND args $<TARGET_FILE:${tgt}>)
    endforeach()

    # file(
    #   GENERATE
    #   OUTPUT ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar
    #   INPUT ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar.in
    # )

    # set(ar_tool ${CMAKE_AR})
    # if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
    #   set(ar_tool ${CMAKE_C_COMPILER_AR})
    # endif()

    add_custom_command(
      COMMAND libtool -static -o ${args}
      OUTPUT ${bundled_tgt_full_name}
      COMMENT "Bundling ${bundled_tgt_name}"
      VERBATIM
      COMMAND_EXPAND_LISTS
    )

    # add_custom_command(
    #   COMMAND ${ar_tool} -M < ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar
    #   OUTPUT ${bundled_tgt_full_name}
    #   COMMENT "Bundling ${bundled_tgt_name}"
    #   VERBATIM
    # )
  elseif(MSVC)
    find_program(lib_tool lib)

    foreach(tgt IN LISTS static_libs)
      list(APPEND static_libs_full_names $<TARGET_FILE:${tgt}>)
    endforeach()

    add_custom_command(
      COMMAND ${lib_tool} /NOLOGO /OUT:${bundled_tgt_full_name} ${static_libs_full_names}
      OUTPUT ${bundled_tgt_full_name}
      COMMENT "Bundling ${bundled_tgt_name}"
      VERBATIM
    )
  else()
    message(FATAL_ERROR "Unknown bundle scenario!")
  endif()

  add_custom_target(bundling_target ALL DEPENDS ${bundled_tgt_full_name})
  add_dependencies(bundling_target ${tgt_name})

  add_library(${bundled_tgt_name} STATIC IMPORTED)
  set_target_properties(
    ${bundled_tgt_name}
    PROPERTIES
    IMPORTED_LOCATION ${bundled_tgt_full_name}
    INTERFACE_INCLUDE_DIRECTORIES $<TARGET_PROPERTY:${tgt_name},INTERFACE_INCLUDE_DIRECTORIES>
  )
  add_dependencies(${bundled_tgt_name} bundling_target)
endfunction()
