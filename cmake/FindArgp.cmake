# FindArgp
# --------
#
# Find argp include dirs and libraries
#
# This module reads hints about search locations from variables::
#
#   ARGP_INCLUDEDIR       - Preferred include directory e.g. <prefix>/include
#   ARGP_LIBRARYDIR       - Preferred library directory e.g. <prefix>/lib
#
# IMPORTED Targets
# ^^^^^^^^^^^^^^^^
#
# This module defines :prop_tgt:`IMPORTED` target ``ARGP::argp``, if
# argp has been found.
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#
# This module defines the following variables::
#
#   ARGP_INCLUDE_DIR - Where to find the header files.
#   ARGP_LIBRARY     - Library when using argp.
#   ARGP_FOUND       - True if argp library is found.

include(CheckFunctionExists)

set(argp_incl_dirs "/usr/include" "/usr/local/include")
if(ARGP_INCLUDEDIR)
  list(APPEND argp_incl_dirs ${ARGP_INCLUDEDIR})
endif()

find_path(
    ARGP_INCLUDE_DIR
    NAMES argp.h
    HINTS ${argp_incl_dirs}
)

set(argp_lib_dirs "/usr/lib" "/usr/local/lib")
if(ARGP_LIBRARYDIR)
    list(APPEND argp_lib_dirs ${ARGP_LIBRARYDIR})
endif()

# First check if argp is shipped together with libc. The required
# argp_parse function should be available after linking to libc,
# otherwise libc doesn't ship it.
check_function_exists("argp_parse" ARGP_IN_LIBC)
if (ARGP_IN_LIBC)
	set(ARGP_LIBRARY "c")
else()
    find_library(
        ARGP_LIBRARY
        NAMES argp argplib libargp
        HINTS ${argp_lib_dirs}
    )
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Argp DEFAULT_MSG
                                  ARGP_LIBRARY ARGP_INCLUDE_DIR)

mark_as_advanced(ARGP_INCLUDE_DIR ARGP_LIBRARY ARGP_FOUND)

if(ARGP_FOUND)
    if(NOT TARGET Argp::argp)
        add_library(Argp::argp UNKNOWN IMPORTED)
        set_target_properties(Argp::argp PROPERTIES
            IMPORTED_LOCATION "${ARGP_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${ARGP_INCLUDE_DIR}")
    endif()
endif()
