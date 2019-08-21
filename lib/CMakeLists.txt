# Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+

add_subdirectory(base)
add_subdirectory(cli)
add_subdirectory(config)
add_subdirectory(remote)
add_subdirectory(icinga)
add_subdirectory(methods)

if(ICINGA2_WITH_CHECKER)
  add_subdirectory(checker)
endif()

if(ICINGA2_WITH_COMPAT)
  add_subdirectory(compat)
endif()

if(ICINGA2_WITH_MYSQL OR ICINGA2_WITH_PGSQL)
  add_subdirectory(db_ido)
endif()

if(ICINGA2_WITH_MYSQL)
  find_package(MySQL)

  if(MYSQL_FOUND)
    add_subdirectory(db_ido_mysql)
    add_subdirectory(mysql_shim)
  else()
    message(FATAL_ERROR "You have selected MySQL support, but MySQL could not be found. You can disable the MySQL IDO module using -DICINGA2_WITH_MYSQL=OFF.")
  endif()
endif()

if(ICINGA2_WITH_PGSQL)
  find_package(PostgreSQL)

  if(PostgreSQL_FOUND)
    add_subdirectory(db_ido_pgsql)
    add_subdirectory(pgsql_shim)
  else()
    message(FATAL_ERROR "You have selected PostgreSQL support, but PostgreSQL could not be found. You can disable the PostgreSQL IDO module using -DICINGA2_WITH_PGSQL=OFF.")
  endif()
endif()

if(ICINGA2_WITH_LIVESTATUS)
  add_subdirectory(livestatus)
endif()

if(ICINGA2_WITH_NOTIFICATION)
  add_subdirectory(notification)
endif()

if(ICINGA2_WITH_PERFDATA)
  add_subdirectory(perfdata)
endif()

if(ICINGA2_WITH_PERL)
  add_subdirectory(perl)
endif()

set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}" PARENT_SCOPE)
