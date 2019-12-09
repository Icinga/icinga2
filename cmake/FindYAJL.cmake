# - Try to find libyajl
# Once done this will define
#  YAJL_FOUND - System has YAJL
#  YAJL_INCLUDE_DIRS - The YAJL include directories
#  YAJL_LIBRARIES - The libraries needed to use YAJL
#  YAJL_DEFINITIONS - Compiler switches required for using YAJL

find_package(PkgConfig)
pkg_check_modules(PC_YAJL QUIET yajl)
set(YAJL_DEFINITIONS ${PC_YAJL_CFLAGS_OTHER})

find_path(YAJL_INCLUDE_DIR yajl/yajl_version.h
          HINTS ${PC_YAJL_INCLUDEDIR} ${PC_YAJL_INCLUDE_DIRS}
          PATH_SUFFIXES libyajl)

find_library(YAJL_LIBRARY NAMES yajl libyajl
             HINTS ${PC_YAJL_LIBDIR} ${PC_YAJL_LIBRARY_DIRS})

set(YAJL_LIBRARIES ${YAJL_LIBRARY} )
set(YAJL_INCLUDE_DIRS ${YAJL_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set YAJL_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(yajl  DEFAULT_MSG
                                  YAJL_LIBRARY YAJL_INCLUDE_DIR)

mark_as_advanced(YAJL_INCLUDE_DIR YAJL_LIBRARY)
