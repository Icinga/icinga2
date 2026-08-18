# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

add_library(db_ido_mysql OBJECT
  idomysqlconnection.cpp idomysqlconnection.hpp idomysqlconnection.ti
)

target_mkclass(db_ido_mysql db_ido_mysql_generated)

target_link_libraries(db_ido_mysql PUBLIC
  common
  MySQL
)

add_dependencies(db_ido_mysql db_ido_generated)

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
