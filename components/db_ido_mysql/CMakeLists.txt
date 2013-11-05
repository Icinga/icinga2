# Icinga 2
# Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)
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

find_package(MYSQL)

if(MYSQL_FOUND)
    mkclass_target(idomysqlconnection.ti idomysqlconnection.th)

    mkembedconfig_target(db_ido_mysql-type.conf db_ido_mysql-type.cpp)

    add_library(db_ido_mysql SHARED idomysqlconnection.cpp idomysqlconnection.th db_ido_mysql-type.cpp)

    include_directories(${MYSQL_INCLUDE_DIR})
    target_link_libraries(db_ido_mysql ${Boost_LIBRARIES} ${MYSQL_LIBRARIES} base config icinga db_ido)

    set_target_properties (
      db_ido_mysql PROPERTIES
      INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/icinga2
      FOLDER Components
    )

    install(
      TARGETS db_ido_mysql
      RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR}
      LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/icinga2
    )
endif()
