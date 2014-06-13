# Icinga 2
# Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)
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

if(UNIX OR CYGWIN)
  configure_file(icinga2-discover-agent.cmake ${CMAKE_CURRENT_BINARY_DIR}/icinga2-discover-agent @ONLY)
  configure_file(icinga2-forget-agent.cmake ${CMAKE_CURRENT_BINARY_DIR}/icinga2-forget-agent @ONLY)
  configure_file(icinga2-list-agents.cmake ${CMAKE_CURRENT_BINARY_DIR}/icinga2-list-agents @ONLY)
  configure_file(icinga2-setup-agent.cmake ${CMAKE_CURRENT_BINARY_DIR}/icinga2-setup-agent @ONLY)
endif()

if(MSVC)
  include_external_msproject(
    icinga2setupagent
    ${CMAKE_CURRENT_SOURCE_DIR}/icinga2-setup-agent/Icinga2SetupAgent.csproj
    TYPE FAE04EC0-301F-11D3-BF4B-00C04F79EFBC
  )

endif()
