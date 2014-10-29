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

add_subdirectory(mkclass)
add_subdirectory(mkembedconfig)
add_subdirectory(mkunity)

configure_file(migrate-hosts.cmake ${CMAKE_CURRENT_BINARY_DIR}/migrate-hosts @ONLY)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/migrate-hosts
  DESTINATION ${CMAKE_INSTALL_DATADIR}/icinga2
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE
)
