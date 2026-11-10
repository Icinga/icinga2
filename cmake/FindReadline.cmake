# SPDX-FileCopyrightText: 2026 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-3.0-or-later

find_path(Readline_INCLUDE_DIRS
  NAMES readline/readline.h
  HINTS ${Readline_ROOT}/include
)

find_library(Readline_LIBRARIES
  NAMES readline
  HINTS ${Readline_ROOT}/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Readline
  REQUIRED_VARS Readline_INCLUDE_DIRS Readline_LIBRARIES
)

if(Readline_FOUND)
  message(STATUS "Readline_LIBRARIES: ${Readline_LIBRARIES}")
  message(STATUS "Readline_INCLUDE_DIRS: ${Readline_INCLUDE_DIRS}")

  add_library(Readline::Readline UNKNOWN IMPORTED)
  set_target_properties(Readline::Readline PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${Readline_INCLUDE_DIRS}"
    IMPORTED_LOCATION             "${Readline_LIBRARIES}"
  )
else()
  set(Readline_LIBRARIES)
  set(Readline_INCLUDE_DIRS)
endif()

mark_as_advanced(Readline_LIBRARIES Readline_INCLUDE_DIRS)
