# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

if(MSVC)
  set(WindowsSources icinga.rc)
else()
  set(WindowsSources "")
endif()

set(icingaloader_SOURCES
  icinga.cpp
  ${WindowsSources}
)

add_library(icingaloader OBJECT ${icingaloader_SOURCES})
add_dependencies(icingaloader base config cli)

set_target_properties (
  icingaloader PROPERTIES
  FOLDER Lib
)

include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

if(ICINGA2_WITH_CHECKER)
  list(APPEND icinga_app_SOURCES $<TARGET_OBJECTS:checker>)
endif()

if(ICINGA2_WITH_COMPAT)
  list(APPEND icinga_app_SOURCES $<TARGET_OBJECTS:compat>)
endif()

if(ICINGA2_WITH_MYSQL OR ICINGA2_WITH_PGSQL)
  list(APPEND icinga_app_SOURCES $<TARGET_OBJECTS:db_ido>)
endif()

if(ICINGA2_WITH_MYSQL)
  list(APPEND icinga_app_SOURCES $<TARGET_OBJECTS:db_ido_mysql>)
endif()

if(ICINGA2_WITH_PGSQL)
  list(APPEND icinga_app_SOURCES $<TARGET_OBJECTS:db_ido_pgsql>)
endif()

if(ICINGA2_WITH_LIVESTATUS)
  list(APPEND icinga_app_SOURCES $<TARGET_OBJECTS:livestatus>)
endif()

if(ICINGA2_WITH_NOTIFICATION)
  list(APPEND icinga_app_SOURCES $<TARGET_OBJECTS:notification>)
endif()

if(ICINGA2_WITH_OPENTELEMETRY)
  list(APPEND icinga_app_SOURCES $<TARGET_OBJECTS:otel>)
endif()

if(ICINGA2_WITH_PERFDATA)
  list(APPEND icinga_app_SOURCES $<TARGET_OBJECTS:perfdata>)
endif()

if(ICINGA2_WITH_ICINGADB)
  list(APPEND icinga_app_SOURCES $<TARGET_OBJECTS:icingadb>)
endif()

add_executable(icinga-app
  $<TARGET_OBJECTS:icingaloader>
  ${base_OBJS}
  $<TARGET_OBJECTS:config>
  $<TARGET_OBJECTS:remote>
  $<TARGET_OBJECTS:cli>
  $<TARGET_OBJECTS:icinga>
  $<TARGET_OBJECTS:methods>
  ${icinga_app_SOURCES}
)

target_link_libraries(icinga-app
  ${base_DEPS}
  cli
)

set_target_properties (
  icinga-app PROPERTIES
  FOLDER Bin
  OUTPUT_NAME icinga2
)

install(
  TARGETS icinga-app
  RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR}
)

if(NOT WIN32)
  install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${ICINGA2_FULL_LOGDIR}\")")
  install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${ICINGA2_FULL_DATADIR}\")")
  install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${ICINGA2_FULL_INITRUNDIR}\")")
endif()
