# Icinga 2
# Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)
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

mkclass_target(icingadb.ti icingadb-ti.cpp icingadb-ti.hpp)

set(icingadb_SOURCES
  icingadb.cpp icingadb-objects.cpp icingadb-stats.cpp icingadb-utility.cpp redisconnection.cpp icingadb-ti.hpp
)

if(ICINGA2_UNITY_BUILD)
    mkunity_target(icingadb icingadb icingadb_SOURCES)
endif()

add_library(icingadb OBJECT ${icingadb_SOURCES})

include_directories(${icinga2_SOURCE_DIR}/third-party)

add_dependencies(icingadb base config icinga remote)

set_target_properties (
  icingadb PROPERTIES
  FOLDER Components
)

install_if_not_exists(
  ${PROJECT_SOURCE_DIR}/etc/icinga2/features-available/icingadb.conf
  ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/features-available
)

set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}" PARENT_SCOPE)
