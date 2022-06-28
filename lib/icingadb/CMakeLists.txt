# Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+

mkclass_target(icingadb.ti icingadb-ti.cpp icingadb-ti.hpp)

mkembedconfig_target(icingadb-itl.conf icingadb-itl.cpp)

set(icingadb_SOURCES
  icingadb.cpp icingadb-objects.cpp icingadb-stats.cpp icingadb-utility.cpp redisconnection.cpp icingadb-ti.hpp
  icingadbchecktask.cpp icingadb-itl.cpp
)

if(ICINGA2_UNITY_BUILD)
    mkunity_target(icingadb icingadb icingadb_SOURCES)
endif()

add_library(icingadb OBJECT ${icingadb_SOURCES})

include_directories(${icinga2_SOURCE_DIR}/third-party)

add_dependencies(icingadb base config icinga remote)

set_target_properties (
  icingadb PROPERTIES
  FOLDER Components
)

install_if_not_exists(
  ${PROJECT_SOURCE_DIR}/etc/icinga2/features-available/icingadb.conf
  ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/features-available
)

set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}" PARENT_SCOPE)
