# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

add_library(db_ido_pgsql OBJECT
  idopgsqlconnection.cpp idopgsqlconnection.hpp idopgsqlconnection.ti
)

target_mkclass(db_ido_pgsql)

target_link_libraries(db_ido_pgsql PUBLIC
  base_interface
  icinga_interface
  remote_interface
  db_ido_interface
  PostgreSQL
)

set_target_properties (
  db_ido_pgsql PROPERTIES
  FOLDER Components
)

install_if_not_exists(
  ${PROJECT_SOURCE_DIR}/etc/icinga2/features-available/ido-pgsql.conf
  ${ICINGA2_CONFIGDIR}/features-available
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
