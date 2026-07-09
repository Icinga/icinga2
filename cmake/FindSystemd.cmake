# SPDX-FileCopyrightText: 2026 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

find_path(Systemd_INCLUDE_DIRS
  NAMES
  systemd/sd-daemon.h
  HINTS ${SYSTEMD_ROOT_DIR}
)

find_library(Systemd_LIBRARIES
  NAMES systemd
  HINTS ${SYSTEMD_LIBRARY_DIR}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Systemd
  REQUIRED_VARS Systemd_LIBRARIES Systemd_INCLUDE_DIRS
)

if(Systemd_FOUND)
  message(STATUS "Systemd_LIBRARIES: ${Systemd_LIBRARIES}")
  message(STATUS "Systemd_INCLUDE_DIRS: ${Systemd_INCLUDE_DIRS}")
  add_library(Systemd UNKNOWN IMPORTED)
  set_target_properties(Systemd PROPERTIES
    IMPORTED_LOCATION "${Systemd_LIBRARIES}"
    INTERFACE_INCLUDE_DIRECTORIES "${Systemd_INCLUDE_DIRS}"
    INTERFACE_COMPILE_DEFINITIONS "HAVE_SYSTEMD"
    INTERFACE_LINK_LIBRARIES "${Systemd_LIBRARIES}"
  )
else()
  set(SYSTEMD_LIBRARIES)
  set(SYSTEMD_INCLUDE_DIRS)
endif()

mark_as_advanced(Systemd_LIBRARIES Systemd_INCLUDE_DIRS)
