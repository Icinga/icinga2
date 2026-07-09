# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

mkclass_target(idomysqlconnection.ti idomysqlconnection-ti.cpp idomysqlconnection-ti.hpp)

add_library(db_ido_mysql OBJECT
  idomysqlconnection.cpp idomysqlconnection.hpp idomysqlconnection-ti.hpp
)

target_link_libraries(db_ido_mysql PUBLIC
  common
  base
  config
  icinga
  db_ido
  MySQL
)

set_target_properties (
  db_ido_mysql PROPERTIES
  FOLDER Components
)

install_if_not_exists(
  ${PROJECT_SOURCE_DIR}/etc/icinga2/features-available/ido-mysql.conf
  ${ICINGA2_CONFIGDIR}/features-available
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
