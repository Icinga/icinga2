# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

add_library(checker OBJECT
  checkercomponent.cpp checkercomponent.hpp checkercomponent.ti
)

target_mkclass(checker checker_generated)

target_link_libraries(checker
  common
)

add_dependencies(checker base_generated icinga_generated remote_generated)

set_target_properties (
  checker PROPERTIES
  FOLDER Components
)

install_if_not_exists(
  ${PROJECT_SOURCE_DIR}/etc/icinga2/features-available/checker.conf
  ${ICINGA2_CONFIGDIR}/features-available
)

if(NOT WIN32)
  install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${ICINGA2_FULL_CONFIGDIR}/features-enabled\")")
  install(CODE "execute_process(COMMAND \"${CMAKE_COMMAND}\" -E create_symlink ../features-available/checker.conf \"\$ENV{DESTDIR}${ICINGA2_FULL_CONFIGDIR}/features-enabled/checker.conf\")")
else()
  install_if_not_exists(${PROJECT_SOURCE_DIR}/etc/icinga2/features-enabled/checker.conf ${ICINGA2_CONFIGDIR}/features-enabled)
endif()

set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}" PARENT_SCOPE)
