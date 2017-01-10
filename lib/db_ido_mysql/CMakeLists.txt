# Icinga 2
# Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)
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

find_package(MySQL)

if(MYSQL_FOUND)
  mkclass_target(idomysqlconnection.ti idomysqlconnection.tcpp idomysqlconnection.thpp)

  set(db_ido_mysql_SOURCES
    idomysqlconnection.cpp idomysqlconnection.thpp
  )

  if(ICINGA2_UNITY_BUILD)
      mkunity_target(db_ido_mysql db_ido_mysql db_ido_mysql_SOURCES)
  endif()

  add_library(db_ido_mysql SHARED ${db_ido_mysql_SOURCES})

  include_directories(${MYSQL_INCLUDE_DIR})
  target_link_libraries(db_ido_mysql ${Boost_LIBRARIES} ${MYSQL_CLIENT_LIBS} base config icinga db_ido)

  set_target_properties (
    db_ido_mysql PROPERTIES
    INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/icinga2
    DEFINE_SYMBOL I2_DB_IDO_MYSQL_BUILD
    FOLDER Components
    VERSION ${SPEC_VERSION}
  )

  install_if_not_exists(
    ${PROJECT_SOURCE_DIR}/etc/icinga2/features-available/ido-mysql.conf
    ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/features-available
  )

  install(
    TARGETS db_ido_mysql
    RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/icinga2
  )

  install(
    DIRECTORY schema
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icinga2-ido-mysql
    FILES_MATCHING PATTERN "*.sql"
  )

  install(
    DIRECTORY schema/upgrade
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icinga2-ido-mysql/schema
    FILES_MATCHING PATTERN "*.sql"
  )

  set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}" PARENT_SCOPE)
else()
  message(FATAL_ERROR "You have selected MySQL support, but MySQL could not be found. You can disable the MySQL IDO module using -DICINGA2_WITH_MYSQL=OFF.")
endif()
