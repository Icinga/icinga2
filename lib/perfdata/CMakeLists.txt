# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

mkclass_target(gelfwriter.ti gelfwriter-ti.cpp gelfwriter-ti.hpp)
mkclass_target(graphitewriter.ti graphitewriter-ti.cpp graphitewriter-ti.hpp)
mkclass_target(influxdbcommonwriter.ti influxdbcommonwriter-ti.cpp influxdbcommonwriter-ti.hpp)
mkclass_target(influxdbwriter.ti influxdbwriter-ti.cpp influxdbwriter-ti.hpp)
mkclass_target(influxdb2writer.ti influxdb2writer-ti.cpp influxdb2writer-ti.hpp)
mkclass_target(elasticsearchwriter.ti elasticsearchwriter-ti.cpp elasticsearchwriter-ti.hpp)
mkclass_target(opentsdbwriter.ti opentsdbwriter-ti.cpp opentsdbwriter-ti.hpp)
mkclass_target(perfdatawriter.ti perfdatawriter-ti.cpp perfdatawriter-ti.hpp)

add_library(perfdata OBJECT
  elasticsearchwriter.cpp elasticsearchwriter.hpp elasticsearchwriter-ti.hpp
  gelfwriter.cpp gelfwriter.hpp gelfwriter-ti.hpp
  graphitewriter.cpp graphitewriter.hpp graphitewriter-ti.hpp
  influxdbcommonwriter.cpp influxdbcommonwriter.hpp influxdbcommonwriter-ti.hpp
  influxdbwriter.cpp influxdbwriter.hpp influxdbwriter-ti.hpp
  influxdb2writer.cpp influxdb2writer.hpp influxdb2writer-ti.hpp
  opentsdbwriter.cpp opentsdbwriter.hpp opentsdbwriter-ti.hpp
  perfdatawriter.cpp perfdatawriter.hpp perfdatawriter-ti.hpp
  perfdatawriterconnection.cpp perfdatawriterconnection.hpp
)

target_link_libraries(perfdata PUBLIC
  common
  base
  icinga
)

if(ICINGA2_WITH_OPENTELEMETRY)
  mkclass_target(otlpmetricswriter.ti otlpmetricswriter-ti.cpp otlpmetricswriter-ti.hpp)
  target_sources(perfdata PRIVATE
    otlpmetricswriter.cpp otlpmetricswriter.hpp otlpmetricswriter-ti.hpp
  )
  target_link_libraries(perfdata PUBLIC otel)
endif()

set_target_properties (
  perfdata PROPERTIES
  FOLDER Components
)

install_if_not_exists(
  ${PROJECT_SOURCE_DIR}/etc/icinga2/features-available/gelf.conf
  ${ICINGA2_CONFIGDIR}/features-available
)

install_if_not_exists(
  ${PROJECT_SOURCE_DIR}/etc/icinga2/features-available/graphite.conf
  ${ICINGA2_CONFIGDIR}/features-available
)

install_if_not_exists(
  ${PROJECT_SOURCE_DIR}/etc/icinga2/features-available/influxdb.conf
  ${ICINGA2_CONFIGDIR}/features-available
)

install_if_not_exists(
  ${PROJECT_SOURCE_DIR}/etc/icinga2/features-available/influxdb2.conf
  ${ICINGA2_CONFIGDIR}/features-available
)

install_if_not_exists(
  ${PROJECT_SOURCE_DIR}/etc/icinga2/features-available/elasticsearch.conf
  ${ICINGA2_CONFIGDIR}/features-available
)

install_if_not_exists(
  ${PROJECT_SOURCE_DIR}/etc/icinga2/features-available/opentsdb.conf
  ${ICINGA2_CONFIGDIR}/features-available
)

if(ICINGA2_WITH_OPENTELEMETRY)
  install_if_not_exists(
    ${PROJECT_SOURCE_DIR}/etc/icinga2/features-available/otlpmetrics.conf
    ${ICINGA2_CONFIGDIR}/features-available
  )
endif()

install_if_not_exists(
  ${PROJECT_SOURCE_DIR}/etc/icinga2/features-available/perfdata.conf
  ${ICINGA2_CONFIGDIR}/features-available
)

install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${ICINGA2_FULL_SPOOLDIR}/perfdata\")")
install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${ICINGA2_FULL_SPOOLDIR}/tmp\")")

set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}" PARENT_SCOPE)
