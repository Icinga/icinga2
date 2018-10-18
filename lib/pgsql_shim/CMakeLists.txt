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

link_directories(${PostgreSQL_LIBRARY_DIRS})
include_directories(${PostgreSQL_INCLUDE_DIRS})

set(pgsql_shim_SOURCES
  pgsql_shim.def
  pgsqlinterface.cpp pgsqlinterface.hpp
)

if(ICINGA2_UNITY_BUILD)
  mkunity_target(pgsql_shim pgsql_shim pgsql_shim_SOURCES)
endif()

add_library(pgsql_shim SHARED ${pgsql_shim_SOURCES})

include(GenerateExportHeader)
generate_export_header(pgsql_shim)

target_link_libraries(pgsql_shim ${PostgreSQL_LIBRARIES})

set_target_properties (
  pgsql_shim PROPERTIES
  FOLDER Lib
  VERSION ${SPEC_VERSION}
)

install(
  TARGETS pgsql_shim
  RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/icinga2
)
