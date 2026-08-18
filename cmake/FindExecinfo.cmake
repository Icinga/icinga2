# SPDX-FileCopyrightText: 2026 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

find_path(Execinfo_INCLUDE_DIRS
  NAMES execinfo.h
  HINTS ${Execinfo_ROOT}/include
)

find_library(Execinfo_LIBRARIES
  NAMES execinfo
  HINTS ${Execinfo_ROOT}/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Execinfo
  REQUIRED_VARS Execinfo_INCLUDE_DIRS Execinfo_LIBRARIES
)

if(Execinfo_FOUND)
  message(STATUS "Execinfo_LIBRARIES: ${Execinfo_LIBRARIES}")
  message(STATUS "Execinfo_INCLUDE_DIRS: ${Execinfo_INCLUDE_DIRS}")

  add_library(Execinfo::Execinfo UNKNOWN IMPORTED)
  set_target_properties(Execinfo::Execinfo PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${Execinfo_INCLUDE_DIRS}"
    IMPORTED_LOCATION             "${Execinfo_LIBRARIES}"
  )
else()
  set(Execinfo_LIBRARIES)
  set(Execinfo_INCLUDE_DIRS)
endif()

mark_as_advanced(Execinfo_LIBRARIES Execinfo_INCLUDE_DIRS)
