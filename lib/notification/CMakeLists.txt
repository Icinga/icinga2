# Icinga 2
# Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)
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

mkclass_target(notificationcomponent.ti notificationcomponent.tcpp notificationcomponent.thpp)

set(notification_SOURCES
  notificationcomponent.cpp notificationcomponent.thpp
)

if(ICINGA2_UNITY_BUILD)
    mkunity_target(notification notification notification_SOURCES)
endif()

add_library(notification SHARED ${notification_SOURCES})

target_link_libraries(notification ${Boost_LIBRARIES} base config icinga)

set_target_properties (
  notification PROPERTIES
  INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/icinga2
  DEFINE_SYMBOL I2_NOTIFICATION_BUILD
  FOLDER Components
  VERSION ${SPEC_VERSION}
)

install_if_not_exists(
  ${PROJECT_SOURCE_DIR}/etc/icinga2/features-available/notification.conf
  ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/features-available
)

if(NOT WIN32)
  install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_SYSCONFDIR}/icinga2/features-enabled\")")
  install(CODE "execute_process(COMMAND \"${CMAKE_COMMAND}\" -E create_symlink ../features-available/notification.conf \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_SYSCONFDIR}/icinga2/features-enabled/notification.conf\")")
else()
  install_if_not_exists(${PROJECT_SOURCE_DIR}/etc/icinga2/features-enabled/notification.conf ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/features-enabled)
endif()

install(TARGETS notification RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/icinga2)

set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}" PARENT_SCOPE)
