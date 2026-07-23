# SPDX-FileCopyrightText: 2026 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

include(FindPackageHandleStandardArgs)

find_path(Execinfo_INC
  NAMES execinfo.h
  HINTS ${Execinfo_ROOT}/include
)

find_library(Execinfo_LIB
  NAMES execinfo
  HINTS ${Execinfo_ROOT}/lib
)

find_package_handle_standard_args(Execinfo
  REQUIRED_VARS Execinfo_INC Execinfo_LIB
)

if(Execinfo_FOUND)
  set(Execinfo_INCLUDE_DIRS ${Execinfo_INC})
  set(Execinfo_LIBRARIES ${Execinfo_LIB})

  add_library(Execinfo UNKNOWN IMPORTED)
  set_target_properties(Execinfo PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${Execinfo_INCLUDE_DIRS}"
    IMPORTED_LOCATION             "${Execinfo_LIBRARIES}"
  )
endif()
