# Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+

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

include_directories(${Boost_INCLUDE_DIRS})

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

if(ICINGA2_WITH_PERFDATA)
  list(APPEND icinga_app_SOURCES $<TARGET_OBJECTS:perfdata>)
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

target_link_libraries(icinga-app ${base_DEPS})

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

install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${ICINGA2_FULL_LOGDIR}\")")
install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${ICINGA2_FULL_DATADIR}\")")
install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${ICINGA2_FULL_INITRUNDIR}\")")
