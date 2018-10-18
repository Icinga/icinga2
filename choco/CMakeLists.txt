# Icinga 2
# Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation
# Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

if(WIN32)
  find_program(CHOCO_BINARY choco)

  configure_file(icinga2.nuspec.cmake icinga2.nuspec)
  configure_file(chocolateyInstall.ps1.cmake chocolateyInstall.ps1)

  add_custom_target(choco-pkg ALL
    COMMAND choco pack
    COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_CURRENT_BINARY_DIR}/icinga2.${SPEC_VERSION}.nupkg ${CMAKE_CURRENT_BINARY_DIR}/icinga2.nupkg
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/icinga2.nuspec ${CMAKE_CURRENT_BINARY_DIR}/chocolateyInstall.ps1 chocolateyUninstall.ps1
  )
endif()
