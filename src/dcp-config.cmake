
####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was dcp-config.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

include(CMakeFindDependencyMacro)
find_dependency(fasta)
find_dependency(hmr)
find_dependency(imm)
find_dependency(Threads)
find_dependency(Argp)
find_dependency(log)
find_dependency(athr)
find_dependency(elapsed)
find_dependency(gff)
find_dependency(tbl)
include("${CMAKE_CURRENT_LIST_DIR}/dcp-targets.cmake")
check_required_components(fasta)
check_required_components(hmr)
check_required_components(imm)
check_required_components(Threads)
check_required_components(Argp)
check_required_components(log)
check_required_components(athr)
check_required_components(elapsed)
check_required_components(gff)
check_required_components(tbl)
