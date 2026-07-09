# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

mkembedconfig_target(icingadb-itl.conf icingadb-itl.cpp)

add_library(icingadb OBJECT
  icingadb.cpp icingadb-objects.cpp icingadb-stats.cpp icingadb-utility.cpp icingadb-worker.cpp redisconnection.cpp icingadb.ti
  icingadbchecktask.cpp icingadb-itl.cpp
)

target_mkclass(icingadb icingadb_generated)

target_link_libraries(icingadb PUBLIC
  common
)

add_dependencies(icingadb base_generated icinga_generated remote_generated)

set_target_properties (
  icingadb PROPERTIES
  FOLDER Components
)

install_if_not_exists(
  ${PROJECT_SOURCE_DIR}/etc/icinga2/features-available/icingadb.conf
  ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/features-available
)

set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}" PARENT_SCOPE)
