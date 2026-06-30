# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

mkclass_target(idopgsqlconnection.ti idopgsqlconnection-ti.cpp idopgsqlconnection-ti.hpp)

set(db_ido_pgsql_SOURCES
  idopgsqlconnection.cpp idopgsqlconnection.hpp idopgsqlconnection-ti.hpp
)

if(ICINGA2_UNITY_BUILD)
  mkunity_target(db_ido_pgsql db_ido_pgsql db_ido_pgsql_SOURCES)
endif()

add_library(db_ido_pgsql OBJECT ${db_ido_pgsql_SOURCES})

include_directories(SYSTEM ${PostgreSQL_INCLUDE_DIRS})

add_dependencies(db_ido_pgsql base config icinga db_ido)

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
