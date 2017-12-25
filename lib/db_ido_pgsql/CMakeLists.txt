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

mkclass_target(idopgsqlconnection.ti idopgsqlconnection.tcpp idopgsqlconnection.thpp)

set(db_ido_pgsql_SOURCES
  idopgsqlconnection.cpp idopgsqlconnection.hpp idopgsqlconnection.thpp
)

if(ICINGA2_UNITY_BUILD)
    mkunity_target(db_ido_pgsql db_ido_pgsql db_ido_pgsql_SOURCES)
endif()

add_library(db_ido_pgsql STATIC ${db_ido_pgsql_SOURCES})

include_directories(${PostgreSQL_INCLUDE_DIRS})

target_link_libraries(db_ido_pgsql PRIVATE base remote icinga db_ido)

set_target_properties (
  db_ido_pgsql PROPERTIES
  FOLDER Components
)

if(ICINGA2_PCH_BUILD)
  target_precompiled_header(db_ido_pgsql REUSE db_ido)
endif()

install_if_not_exists(
  ${PROJECT_SOURCE_DIR}/etc/icinga2/features-available/ido-pgsql.conf
  ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/features-available
)

install(
  DIRECTORY schema
  DESTINATION ${CMAKE_INSTALL_DATADIR}/icinga2-ido-pgsql
  FILES_MATCHING PATTERN "*.sql"
)

install(
  DIRECTORY schema/upgrade
  DESTINATION ${CMAKE_INSTALL_DATADIR}/icinga2-ido-pgsql/schema
  FILES_MATCHING PATTERN "*.sql"
)

set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}" PARENT_SCOPE)
