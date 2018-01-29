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

if(NOT WIN32)
  configure_file(icinga2/constants.conf.cmake ${CMAKE_CURRENT_BINARY_DIR}/icinga2/constants.conf @ONLY)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  configure_file(logrotate.d/icinga2.cmake ${CMAKE_CURRENT_BINARY_DIR}/logrotate.d/icinga2 @ONLY)
endif()

if(NOT WIN32)
  install_if_not_exists(${CMAKE_CURRENT_BINARY_DIR}/icinga2/constants.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2)
	install_if_not_exists(icinga2/icinga2.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2)
else()
  install_if_not_exists(${CMAKE_CURRENT_SOURCE_DIR}/icinga2/win32/constants.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2)
		install_if_not_exists(icinga2/win32/icinga2.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2)
endif()

install_if_not_exists(icinga2/zones.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2)
install_if_not_exists(icinga2/conf.d/app.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/conf.d)
install_if_not_exists(icinga2/conf.d/commands.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/conf.d)
install_if_not_exists(icinga2/conf.d/downtimes.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/conf.d)
install_if_not_exists(icinga2/conf.d/groups.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/conf.d)

if(NOT WIN32)
  install_if_not_exists(icinga2/conf.d/hosts.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/conf.d)
	install_if_not_exists(icinga2/conf.d/services.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/conf.d)
else()
  install_if_not_exists(icinga2/conf.d/win32/hosts.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/conf.d)
	install_if_not_exists(icinga2/conf.d/win32/services.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/conf.d)
endif()

install_if_not_exists(icinga2/conf.d/notifications.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/conf.d)
install_if_not_exists(icinga2/conf.d/templates.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/conf.d)
install_if_not_exists(icinga2/conf.d/timeperiods.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/conf.d)
install_if_not_exists(icinga2/conf.d/users.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/conf.d)
install_if_not_exists(icinga2/features-available/api.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/features-available)
install_if_not_exists(icinga2/features-available/debuglog.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/features-available)
install_if_not_exists(icinga2/features-available/mainlog.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/features-available)
if(NOT WIN32)
  install_if_not_exists(icinga2/features-available/syslog.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/features-available)
endif()
install_if_not_exists(icinga2/scripts/mail-host-notification.sh ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/scripts)
install_if_not_exists(icinga2/scripts/mail-service-notification.sh ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/scripts)
install_if_not_exists(icinga2/zones.d/README ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/zones.d)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  install_if_not_exists(${CMAKE_CURRENT_BINARY_DIR}/logrotate.d/icinga2 ${CMAKE_INSTALL_SYSCONFDIR}/logrotate.d)
endif()

if(NOT WIN32)
  install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_SYSCONFDIR}/icinga2/features-enabled\")")
  install(CODE "execute_process(COMMAND \"${CMAKE_COMMAND}\" -E create_symlink ../features-available/mainlog.conf \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_SYSCONFDIR}/icinga2/features-enabled/mainlog.conf\")")

  install(
    FILES bash_completion.d/icinga2
    DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/bash_completion.d
  )
else()
  install_if_not_exists(icinga2/features-enabled/mainlog.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/features-enabled)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "(Linux|Solaris|SunOS)")
  add_subdirectory(initsystem)
endif()

set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}" PARENT_SCOPE)
