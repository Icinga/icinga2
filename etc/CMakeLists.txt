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

include(InstallConfig)

configure_file(icinga/icinga-classic-apache.conf.cmake ${CMAKE_CURRENT_BINARY_DIR}/icinga/icinga-classic-apache.conf @ONLY)
configure_file(logrotate.d/icinga2.cmake ${CMAKE_CURRENT_BINARY_DIR}/logrotate.d/icinga2 @ONLY)

install_if_not_exists(icinga2/icinga2.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2)
install_if_not_exists(icinga2/constants.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2)
install_if_not_exists(icinga2/zones.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2)
install_if_not_exists(icinga2/conf.d/commands.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/conf.d)
install_if_not_exists(icinga2/conf.d/downtimes.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/conf.d)
install_if_not_exists(icinga2/conf.d/groups.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/conf.d)
install_if_not_exists(icinga2/conf.d/notifications.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/conf.d)
install_if_not_exists(icinga2/conf.d/services.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/conf.d)
install_if_not_exists(icinga2/conf.d/templates.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/conf.d)
install_if_not_exists(icinga2/conf.d/timeperiods.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/conf.d)
install_if_not_exists(icinga2/conf.d/users.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/conf.d)
install_if_not_exists(icinga2/conf.d/hosts/localhost.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/conf.d/hosts)
install_if_not_exists(icinga2/conf.d/hosts/localhost/disk.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/conf.d/hosts/localhost)
install_if_not_exists(icinga2/conf.d/hosts/localhost/http.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/conf.d/hosts/localhost)
install_if_not_exists(icinga2/conf.d/hosts/localhost/icinga.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/conf.d/hosts/localhost)
install_if_not_exists(icinga2/conf.d/hosts/localhost/kernel.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/conf.d/hosts/localhost)
install_if_not_exists(icinga2/conf.d/hosts/localhost/load.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/conf.d/hosts/localhost)
install_if_not_exists(icinga2/conf.d/hosts/localhost/procs.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/conf.d/hosts/localhost)
install_if_not_exists(icinga2/conf.d/hosts/localhost/ssh.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/conf.d/hosts/localhost)
install_if_not_exists(icinga2/conf.d/hosts/localhost/swap.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/conf.d/hosts/localhost)
install_if_not_exists(icinga2/conf.d/hosts/localhost/users.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/conf.d/hosts/localhost)
install_if_not_exists(icinga2/features-available/api.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/features-available)
install_if_not_exists(icinga2/features-available/checker.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/features-available)
install_if_not_exists(icinga2/features-available/command.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/features-available)
install_if_not_exists(icinga2/features-available/compatlog.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/features-available)
install_if_not_exists(icinga2/features-available/debuglog.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/features-available)
install_if_not_exists(icinga2/features-available/graphite.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/features-available)
install_if_not_exists(icinga2/features-available/icingastatus.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/features-available)
install_if_not_exists(icinga2/features-available/ido-mysql.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/features-available)
install_if_not_exists(icinga2/features-available/ido-pgsql.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/features-available)
install_if_not_exists(icinga2/features-available/livestatus.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/features-available)
install_if_not_exists(icinga2/features-available/mainlog.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/features-available)
install_if_not_exists(icinga2/features-available/notification.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/features-available)
install_if_not_exists(icinga2/features-available/perfdata.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/features-available)
install_if_not_exists(icinga2/features-available/statusdata.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/features-available)
install_if_not_exists(icinga2/features-available/syslog.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/features-available)
install_if_not_exists(icinga2/scripts/check_kernel ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/scripts)
install_if_not_exists(icinga2/scripts/mail-host-notification.sh ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/scripts)
install_if_not_exists(icinga2/scripts/mail-service-notification.sh ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/scripts)
install_if_not_exists(icinga2/zones.d/README ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/zones.d)
install_if_not_exists(${CMAKE_CURRENT_BINARY_DIR}/logrotate.d/icinga2 ${CMAKE_INSTALL_SYSCONFDIR}/logrotate.d)

install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_SYSCONFDIR}/icinga2/pki\")")

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

set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}" PARENT_SCOPE)
