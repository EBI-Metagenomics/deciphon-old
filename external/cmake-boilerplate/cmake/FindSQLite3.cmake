# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindSQLite3
-----------

.. versionadded:: 3.14

Find the SQLite libraries, v3

IMPORTED targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` target:

``SQLite::SQLite3``

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables if found:

``SQLite3_INCLUDE_DIRS``
  where to find sqlite3.h, etc.
``SQLite3_LIBRARIES``
  the libraries to link against to use SQLite3.
``SQLite3_VERSION``
  version of the SQLite3 library found
``SQLite3_FOUND``
  TRUE if found

# Options
# ^^^^^^^
# ``SQLite3_USE_STATIC_LIBS``
#   Set to ON to force the use of the static library.  Default is ``OFF``.

#]=======================================================================]

# Support preference of static libs by adjusting CMAKE_FIND_LIBRARY_SUFFIXES
if(SQLite3_USE_STATIC_LIBS)
  set(_sqlite3_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
  if(WIN32)
    list(INSERT CMAKE_FIND_LIBRARY_SUFFIXES 0 .lib .a)
  else()
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
  endif()
endif()

# Look for the necessary header
find_path(
  SQLite3_INCLUDE_DIR NAME sqlite3.h PATH_SUFFIXES include
  HINTS /usr/include /usr/local/include NO_CMAKE_SYSTEM_PATH
)
find_library(
  SQLite3_LIBRARY NAMES sqlite3 sqlite PATH_SUFFIXES lib
  HINTS /usr/lib /usr/local/lib NO_CMAKE_SYSTEM_PATH
)

mark_as_advanced(SQLite3_INCLUDE_DIR)

# Extract version information from the header file
if(SQLite3_INCLUDE_DIR)
  file(
    STRINGS ${SQLite3_INCLUDE_DIR}/sqlite3.h _ver_line
    REGEX "^#define SQLITE_VERSION  *\"[0-9]+\\.[0-9]+\\.[0-9]+\""
    LIMIT_COUNT 1
  )
  string(
    REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+"
    SQLite3_VERSION "${_ver_line}"
  )
  unset(_ver_line)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  SQLite3
  REQUIRED_VARS SQLite3_INCLUDE_DIR SQLite3_LIBRARY
  VERSION_VAR SQLite3_VERSION
)

# Create the imported target
if(SQLite3_FOUND)
  set(SQLite3_INCLUDE_DIRS ${SQLite3_INCLUDE_DIR})
  set(SQLite3_LIBRARIES ${SQLite3_LIBRARY})
  if(NOT TARGET SQLite::SQLite3)
    if(SQLite3_USE_STATIC_LIBS)
      add_library(SQLite::SQLite3 STATIC IMPORTED)
    else()
      add_library(SQLite::SQLite3 UNKNOWN IMPORTED)
    endif()
    set_target_properties(
      SQLite::SQLite3 PROPERTIES
      IMPORTED_LOCATION "${SQLite3_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${SQLite3_INCLUDE_DIR}"
    )
  endif()
endif()

# Restore the original find library ordering
if(sqlite3_USE_STATIC_LIBS)
  set(CMAKE_FIND_LIBRARY_SUFFIXES ${_sqlite3_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})
endif()
