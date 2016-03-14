# Icinga 2
# Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)
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

if(MSVC)
  include_external_msproject(
    icinga2setupagent
    ${CMAKE_CURRENT_SOURCE_DIR}/windows-setup-agent/Icinga2SetupAgent.csproj
    TYPE FAE04EC0-301F-11D3-BF4B-00C04F79EFBC
    PLATFORM Win32
  )

  install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/windows-setup-agent/bin/\${CMAKE_INSTALL_CONFIG_NAME}/Icinga2SetupAgent.exe ${CMAKE_CURRENT_SOURCE_DIR}/windows-setup-agent/bin/\${CMAKE_INSTALL_CONFIG_NAME}/Icinga2SetupAgent.exe.config
    DESTINATION ${CMAKE_INSTALL_SBINDIR}
  )
endif()
