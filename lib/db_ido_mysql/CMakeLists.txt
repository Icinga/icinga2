# Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+

mkclass_target(idomysqlconnection.ti idomysqlconnection-ti.cpp idomysqlconnection-ti.hpp)

set(db_ido_mysql_SOURCES
  idomysqlconnection.cpp idomysqlconnection.hpp idomysqlconnection-ti.hpp
)

if(ICINGA2_UNITY_BUILD)
  mkunity_target(db_ido_mysql db_ido_mysql db_ido_mysql_SOURCES)
endif()

add_library(db_ido_mysql OBJECT ${db_ido_mysql_SOURCES})

include_directories(SYSTEM ${MYSQL_INCLUDE_DIR})

add_dependencies(db_ido_mysql base config icinga db_ido)

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
