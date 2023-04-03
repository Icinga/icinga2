# Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+

mkclass_target(compatlogger.ti compatlogger-ti.cpp compatlogger-ti.hpp)
mkclass_target(externalcommandlistener.ti externalcommandlistener-ti.cpp externalcommandlistener-ti.hpp)

set(compat_SOURCES
  compatlogger.cpp compatlogger.hpp compatlogger-ti.hpp
  externalcommandlistener.cpp externalcommandlistener.hpp externalcommandlistener-ti.hpp
)

if(ICINGA2_UNITY_BUILD)
  mkunity_target(compat compat compat_SOURCES)
endif()

add_library(compat OBJECT ${compat_SOURCES})

add_dependencies(compat base config icinga)

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
