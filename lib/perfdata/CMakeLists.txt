# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

add_library(perfdata OBJECT
  elasticsearchwriter.cpp elasticsearchwriter.hpp elasticsearchwriter.ti
  gelfwriter.cpp gelfwriter.hpp gelfwriter.ti
  graphitewriter.cpp graphitewriter.hpp graphitewriter.ti
  influxdbcommonwriter.cpp influxdbcommonwriter.hpp influxdbcommonwriter.ti
  influxdbwriter.cpp influxdbwriter.hpp influxdbwriter.ti
  influxdb2writer.cpp influxdb2writer.hpp influxdb2writer.ti
  opentsdbwriter.cpp opentsdbwriter.hpp opentsdbwriter.ti
  perfdatawriter.cpp perfdatawriter.hpp perfdatawriter.ti
  perfdatawriterconnection.cpp perfdatawriterconnection.hpp
)

target_link_libraries(perfdata PUBLIC
  common
)

add_dependencies(perfdata base_generated icinga_generated)

if(ICINGA2_WITH_OPENTELEMETRY)
  target_sources(perfdata PRIVATE
    otlpmetricswriter.cpp otlpmetricswriter.hpp otlpmetricswriter.ti
  )
  target_link_libraries(perfdata PUBLIC otel)
endif()

target_mkclass(perfdata perfdata_generated)

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
