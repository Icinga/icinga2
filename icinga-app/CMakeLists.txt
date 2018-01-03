# Icinga 2
# Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation
# Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

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

include_directories(${Boost_INCLUDE_DIRS})

add_executable(icinga-app $<TARGET_OBJECTS:icingaloader>)
target_link_libraries(icinga-app PRIVATE ${Boost_PROGRAM_OPTIONS_LIBRARY})
add_whole_static_library(icinga-app base)
add_whole_static_library(icinga-app config)
add_whole_static_library(icinga-app remote)
add_whole_static_library(icinga-app cli)

if(ICINGA2_WITH_CHECKER)
  add_whole_static_library(icinga-app checker)
endif()

if(ICINGA2_WITH_COMPAT)
  add_whole_static_library(icinga-app compat)
endif()

if(ICINGA2_WITH_MYSQL)
  add_whole_static_library(icinga-app db_ido_mysql)
endif()

if(ICINGA2_WITH_PGSQL)
  add_whole_static_library(icinga-app db_ido_pgsql)
endif()

add_whole_static_library(icinga-app icinga)

if(ICINGA2_WITH_LIVESTATUS)
  add_whole_static_library(icinga-app livestatus)
endif()

add_whole_static_library(icinga-app methods)

if(ICINGA2_WITH_NOTIFICATION)
  add_whole_static_library(icinga-app notification)
endif()

if(ICINGA2_WITH_PERFDATA)
  add_whole_static_library(icinga-app perfdata)
endif()

set_target_properties (
  icinga-app PROPERTIES
  INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/icinga2
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

if(ICINGA2_WITH_HELLO)
  add_executable(hello-app $<TARGET_OBJECTS:icingaloader>)
  target_link_libraries(hello-app PRIVATE ${Boost_PROGRAM_OPTIONS_LIBRARY})
  add_whole_static_library(hello-app base)
  add_whole_static_library(hello-app config)
  add_whole_static_library(hello-app remote)
  add_whole_static_library(hello-app cli)
  add_whole_static_library(hello-app hello)

  set_target_properties (
    hello-app PROPERTIES
    INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/icinga2
    FOLDER Bin
    OUTPUT_NAME hello
  )

  install(
    TARGETS hello-app
    RUNTIME DESTINATION ${InstallPath}
  )
endif()

install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/log/icinga2\")")
install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/lib/icinga2\")")
install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${ICINGA2_RUNDIR}/icinga2\")")
