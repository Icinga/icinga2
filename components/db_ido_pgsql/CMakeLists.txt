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

find_package(PostgreSQL)

if(PostgreSQL_FOUND)
    mkclass_target(idopgsqlconnection.ti idopgsqlconnection.thpp)

    mkembedconfig_target(db_ido_pgsql-type.conf db_ido_pgsql-type.cpp)

    link_directories(${PostgreSQL_LIBRARY_DIRS})
    include_directories(${PostgreSQL_INCLUDE_DIRS})

    add_library(db_ido_pgsql SHARED idopgsqlconnection.cpp idopgsqlconnection.thpp db_ido_pgsql-type.cpp)

    target_link_libraries(db_ido_pgsql ${Boost_LIBRARIES} ${PostgreSQL_LIBRARIES} base config icinga db_ido)

    set_target_properties (
      db_ido_pgsql PROPERTIES
      INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/icinga2
      FOLDER Components
    )

    install(
      TARGETS db_ido_pgsql
      RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR}
      LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/icinga2
    )
endif()
