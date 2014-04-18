# Icinga 2
# Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)
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

configure_file(icinga/icinga-classic-apache.conf.cmake ${CMAKE_CURRENT_BINARY_DIR}/icinga/icinga-classic-apache.conf)

install(
  FILES icinga2/icinga2.conf icinga2/constants.conf
  DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/icinga2
)

install(
  FILES icinga2/conf.d/commands.conf icinga2/conf.d/downtimes.conf icinga2/conf.d/generic-host.conf
        icinga2/conf.d/generic-service.conf icinga2/conf.d/generic-user.conf icinga2/conf.d/groups.conf
        icinga2/conf.d/notifications.conf icinga2/conf.d/timeperiods.conf icinga2/conf.d/users.conf
  DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/conf.d
)

install(
  FILES icinga2/conf.d/hosts/localhost.conf
  DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/conf.d/hosts
)

install(
  FILES icinga2/conf.d/hosts/localhost/disk.conf icinga2/conf.d/hosts/localhost/http.conf
        icinga2/conf.d/hosts/localhost/icinga.conf icinga2/conf.d/hosts/localhost/kernel.conf
        icinga2/conf.d/hosts/localhost/load.conf icinga2/conf.d/hosts/localhost/processes.conf
        icinga2/conf.d/hosts/localhost/ssh.conf icinga2/conf.d/hosts/localhost/swap.conf
        icinga2/conf.d/hosts/localhost/users.conf
  DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/conf.d/hosts/localhost
)

install(
  FILES icinga2/features-available/agent.conf icinga2/features-available/checker.conf
        icinga2/features-available/command.conf icinga2/features-available/compatlog.conf
        icinga2/features-available/debuglog.conf icinga2/features-available/graphite.conf
        icinga2/features-available/ido-mysql.conf icinga2/features-available/ido-pgsql.conf
        icinga2/features-available/livestatus.conf icinga2/features-available/mainlog.conf
        icinga2/features-available/notification.conf icinga2/features-available/perfdata.conf
        icinga2/features-available/statusdata.conf icinga2/features-available/syslog.conf
  DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/features-available
)

install(
  FILES icinga2/scripts/check_kernel icinga2/scripts/mail-host-notification.sh
        icinga2/scripts/mail-service-notification.sh
  DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/scripts
)

install(
  FILES logrotate.d/icinga2
  DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/logrotate.d
)

if(NOT WIN32)
  install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_SYSCONFDIR}/icinga2/features-enabled\")")
  install(CODE "execute_process(COMMAND \"${CMAKE_COMMAND}\" -E create_symlink ../features-available/checker.conf \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_SYSCONFDIR}/icinga2/features-enabled/checker.conf\")")
  install(CODE "execute_process(COMMAND \"${CMAKE_COMMAND}\" -E create_symlink ../features-available/notification.conf \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_SYSCONFDIR}/icinga2/features-enabled/notification.conf\")")
  install(CODE "execute_process(COMMAND \"${CMAKE_COMMAND}\" -E create_symlink ../features-available/mainlog.conf \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_SYSCONFDIR}/icinga2/features-enabled/mainlog.conf\")")
else()
  install(
    FILES icinga2/features-enabled/checker.conf icinga2/features-enabled/notification.conf
          icinga2/features-enabled/mainlog.conf
    DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/features-enabled
  )
endif()

if(NOT WIN32)
  configure_file(init.d/icinga2.cmake ${CMAKE_CURRENT_BINARY_DIR}/init.d/icinga2)
  install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/init.d/icinga2
    DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/init.d
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
  )
endif()
