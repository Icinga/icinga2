# Icinga 2
# Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)
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

mkclass_target(notificationcomponent.ti notificationcomponent-ti.cpp notificationcomponent-ti.hpp)

set(notification_SOURCES
  notificationcomponent.cpp notificationcomponent.hpp notificationcomponent-ti.hpp
)

if(ICINGA2_UNITY_BUILD)
  mkunity_target(notification notification notification_SOURCES)
endif()

add_library(notification OBJECT ${notification_SOURCES})

add_dependencies(notification base config icinga)

set_target_properties (
  notification PROPERTIES
  FOLDER Components
)

install_if_not_exists(
  ${PROJECT_SOURCE_DIR}/etc/icinga2/features-available/notification.conf
  ${ICINGA2_CONFIGDIR}/features-available
)

if(NOT WIN32)
  install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${ICINGA2_FULL_CONFIGDIR}/features-enabled\")")
  install(CODE "execute_process(COMMAND \"${CMAKE_COMMAND}\" -E create_symlink ../features-available/notification.conf \"\$ENV{DESTDIR}${ICINGA2_FULL_CONFIGDIR}/features-enabled/notification.conf\")")
else()
  install_if_not_exists(${PROJECT_SOURCE_DIR}/etc/icinga2/features-enabled/notification.conf ${ICINGA2_CONFIGDIR}/features-enabled)
endif()

set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}" PARENT_SCOPE)
