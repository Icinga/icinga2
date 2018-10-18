# Icinga 2
# Copyright (C) 2018 Icinga Development Team (https://icinga.com)
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation
# Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

# Ensures a directory is absolute by prefixing CMAKE_INSTALL_PREFIX if it is not
# similar to CMAKE_INSTALL_FULL_... https://cmake.org/cmake/help/latest/module/GNUInstallDirs.html
function(set_full_dir var path)
  if(NOT IS_ABSOLUTE "${path}")
    message(STATUS "Prefixing in ${var} \"${path}\" with ${CMAKE_INSTALL_PREFIX}")
    set(path "${CMAKE_INSTALL_PREFIX}/${path}")
  endif()
  set(${var} "${path}" PARENT_SCOPE)
endfunction(set_full_dir)
