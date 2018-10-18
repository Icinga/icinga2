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

include_directories(${MYSQL_INCLUDE_DIR})

set(mysql_shim_SOURCES
  mysql_shim.def
  mysqlinterface.cpp mysqlinterface.hpp
)

if(ICINGA2_UNITY_BUILD)
  mkunity_target(mysql_shim mysql_shim mysql_shim_SOURCES)
endif()

add_library(mysql_shim SHARED ${mysql_shim_SOURCES})

include(GenerateExportHeader)
generate_export_header(mysql_shim)

target_link_libraries(mysql_shim ${MYSQL_LIB})

set_target_properties (
  mysql_shim PROPERTIES
  FOLDER Lib
  VERSION ${SPEC_VERSION}
)

install(
  TARGETS mysql_shim
  RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/icinga2
)
