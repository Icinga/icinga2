# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

add_executable(icinga-app
  icinga.cpp
)

target_link_libraries(icinga-app PUBLIC
  base
  config
  cli
  icinga
  methods
  remote
  Boost::program_options
)

# TODO: MSVC same as WIN32 or is mingwc a reasonable possibility?
if(MSVC)
  target_sources(icinga-app PRIVATE
    icinga.rc
  )
endif()

if(ICINGA2_WITH_CHECKER)
  target_link_libraries(icinga-app PUBLIC checker)
endif()

if(ICINGA2_WITH_COMPAT)
  target_link_libraries(icinga-app PUBLIC compat)
endif()

if(ICINGA2_WITH_MYSQL OR ICINGA2_WITH_PGSQL)
  target_link_libraries(icinga-app PUBLIC db_ido)
endif()

if(ICINGA2_WITH_MYSQL)
  target_link_libraries(icinga-app PUBLIC db_ido_mysql)
endif()

if(ICINGA2_WITH_PGSQL)
  target_link_libraries(icinga-app PUBLIC db_ido_pgsql)
endif()

if(ICINGA2_WITH_LIVESTATUS)
  target_link_libraries(icinga-app PUBLIC livestatus)
endif()

if(ICINGA2_WITH_NOTIFICATION)
  target_link_libraries(icinga-app PUBLIC notification)
endif()

if(ICINGA2_WITH_OPENTELEMETRY)
  target_link_libraries(icinga-app PUBLIC otel)
endif()

if(ICINGA2_WITH_PERFDATA)
  target_link_libraries(icinga-app PUBLIC perfdata)
endif()

if(ICINGA2_WITH_ICINGADB)
  target_link_libraries(icinga-app PUBLIC icingadb)
endif()

set_target_properties (
  icinga-app PROPERTIES
  FOLDER Bin
  OUTPUT_NAME icinga2
)

if(WIN32)
  set(InstallPath "${CMAKE_INSTALL_SBINDIR}")
else()
  configure_file(icinga2.cmake ${CMAKE_CURRENT_BINARY_DIR}/icinga2 @ONLY)

  install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/icinga2
    DESTINATION ${CMAKE_INSTALL_SBINDIR}
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
  )

  set(InstallPath "${CMAKE_INSTALL_LIBDIR}/icinga2/sbin")
endif()

install(
  TARGETS icinga-app
  RUNTIME DESTINATION ${InstallPath}
)

if(NOT WIN32)
  install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${ICINGA2_FULL_LOGDIR}\")")
  install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${ICINGA2_FULL_DATADIR}\")")
  install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${ICINGA2_FULL_INITRUNDIR}\")")
endif()
