# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

add_library(compat OBJECT
  compatlogger.cpp compatlogger.hpp compatlogger.ti
  externalcommandlistener.cpp externalcommandlistener.hpp externalcommandlistener.ti
)

target_mkclass(compat compat_generated)

target_link_libraries(compat PUBLIC
  common
)

add_dependencies(compat base_generated icinga_generated)

set_target_properties (
  compat PROPERTIES
  FOLDER Components
)

install_if_not_exists(
  ${PROJECT_SOURCE_DIR}/etc/icinga2/features-available/command.conf
  ${ICINGA2_CONFIGDIR}/features-available
)

install_if_not_exists(
  ${PROJECT_SOURCE_DIR}/etc/icinga2/features-available/compatlog.conf
  ${ICINGA2_CONFIGDIR}/features-available
)

install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${ICINGA2_FULL_LOGDIR}/compat/archives\")")
install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${ICINGA2_FULL_SPOOLDIR}\")")
install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${ICINGA2_FULL_INITRUNDIR}/cmd\")")

set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}" PARENT_SCOPE)
