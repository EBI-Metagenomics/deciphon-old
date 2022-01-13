# FindxxHash
# -----------
#
# Find the xxHash library header and define variables.
#
# Imported Targets
# ^^^^^^^^^^^^^^^^
#
# This module defines :prop_tgt:`IMPORTED` target ``xxHash::xxHash``,
# if xxHash has been found
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#
# This module defines the following variables:
#
# ::
#
#   xxHash_FOUND          - True if xxHash is found.
#   xxHash_INCLUDE_DIRS   - Where to find xxhash.h
#
# ::
#
#   xxHash_VERSION        - The version of xxHash found (x.y.z)
#   xxHash_VERSION_MAJOR  - The major version of xxHash
#   xxHash_VERSION_MINOR  - The minor version of xxHash
#   xxHash_VERSION_PATCH  - The patch version of xxHash
#
# Options
# ^^^^^^^
# ``xxHash_USE_STATIC_LIBS``
#   Set to ON to force the use of the static library.  Default is ``OFF``.

# Support preference of static libs by adjusting CMAKE_FIND_LIBRARY_SUFFIXES
if(xxHash_USE_STATIC_LIBS)
  set(_xxhash_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
  if(WIN32)
    list(INSERT CMAKE_FIND_LIBRARY_SUFFIXES 0 .lib .a)
  else()
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
  endif()
endif()

find_path(xxHash_INCLUDE_DIR NAME xxhash.h PATH_SUFFIXES include)
find_library(xxHash_LIBRARY NAMES xxhash xxHash PATH_SUFFIXES lib)

mark_as_advanced(xxHash_INCLUDE_DIR)

if(xxHash_INCLUDE_DIR AND EXISTS "${xxHash_INCLUDE_DIR}/xxhash.h")
  file(STRINGS "${xxHash_INCLUDE_DIR}/xxhash.h" XXHASH_H REGEX "^#define XXH_VERSION_[A-Z]+[ ]+[0-9]+$")
  string(REGEX REPLACE ".+XXH_VERSION_MAJOR[ ]+([0-9]+).*$" "\\1" xxHash_VERSION_MAJOR "${XXHASH_H}")
  string(REGEX REPLACE ".+XXH_VERSION_MINOR[ ]+([0-9]+).*$" "\\1" xxHash_VERSION_MINOR "${XXHASH_H}")
  string(REGEX REPLACE ".+XXH_VERSION_RELEASE[ ]+([0-9]+).*$" "\\1" xxHash_VERSION_PATCH "${XXHASH_H}")
  set(xxHash_VERSION "${xxHash_VERSION_MAJOR}.${xxHash_VERSION_MINOR}.${xxHash_VERSION_PATCH}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  xxHash
  REQUIRED_VARS xxHash_LIBRARY xxHash_INCLUDE_DIR VERSION_VAR xxHash_VERSION
)

if(xxHash_FOUND)
  set(xxHash_INCLUDE_DIRS "${xxHash_INCLUDE_DIR}")

  if(NOT xxHash_LIBRARIES)
    set(xxHash_LIBRARIES ${xxHash_LIBRARY})
  endif()

  if(NOT TARGET xxHash::xxHash)
    if(xxHash_USE_STATIC_LIBS)
      add_library(xxHash::xxHash STATIC IMPORTED)
    else()
      add_library(xxHash::xxHash UNKNOWN IMPORTED)
    endif()
    set_target_properties(
      xxHash::xxHash PROPERTIES
      IMPORTED_LOCATION "${xxHash_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${xxHash_INCLUDE_DIRS}"
    )
  endif()
endif()

# Restore the original find library ordering
if(xxHash_USE_STATIC_LIBS)
  set(CMAKE_FIND_LIBRARY_SUFFIXES ${_xxhash_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})
endif()
