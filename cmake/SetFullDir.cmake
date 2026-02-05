# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Ensures a directory is absolute by prefixing CMAKE_INSTALL_PREFIX if it is not
# similar to CMAKE_INSTALL_FULL_... https://cmake.org/cmake/help/latest/module/GNUInstallDirs.html
function(set_full_dir var path)
  if(NOT IS_ABSOLUTE "${path}")
    message(STATUS "Prefixing in ${var} \"${path}\" with ${CMAKE_INSTALL_PREFIX}")
    set(path "${CMAKE_INSTALL_PREFIX}/${path}")
  endif()
  set(${var} "${path}" PARENT_SCOPE)
endfunction(set_full_dir)
