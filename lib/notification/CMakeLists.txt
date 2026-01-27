# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

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
