# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

mkclass_target(livestatuslistener.ti livestatuslistener-ti.cpp livestatuslistener-ti.hpp)

set(livestatus_SOURCES
  i2-livestatus.hpp
  aggregator.cpp aggregator.hpp
  andfilter.cpp andfilter.hpp
  attributefilter.cpp attributefilter.hpp
  avgaggregator.cpp avgaggregator.hpp
  column.cpp column.hpp
  combinerfilter.cpp combinerfilter.hpp
  commandstable.cpp commandstable.hpp
  commentstable.cpp commentstable.hpp
  contactgroupstable.cpp contactgroupstable.hpp
  contactstable.cpp contactstable.hpp
  countaggregator.cpp countaggregator.hpp
  downtimestable.cpp downtimestable.hpp
  endpointstable.cpp endpointstable.hpp
  filter.hpp
  historytable.hpp
  hostgroupstable.cpp hostgroupstable.hpp
  hoststable.cpp hoststable.hpp
  invavgaggregator.cpp invavgaggregator.hpp
  invsumaggregator.cpp invsumaggregator.hpp
  livestatuslistener.cpp livestatuslistener.hpp livestatuslistener-ti.hpp
  livestatuslogutility.cpp livestatuslogutility.hpp
  livestatusquery.cpp livestatusquery.hpp
  logtable.cpp logtable.hpp
  maxaggregator.cpp maxaggregator.hpp
  minaggregator.cpp minaggregator.hpp
  negatefilter.cpp negatefilter.hpp
  orfilter.cpp orfilter.hpp
  servicegroupstable.cpp servicegroupstable.hpp
  servicestable.cpp servicestable.hpp
  statehisttable.cpp statehisttable.hpp
  statustable.cpp statustable.hpp
  stdaggregator.cpp stdaggregator.hpp
  sumaggregator.cpp sumaggregator.hpp
  table.cpp table.hpp
  timeperiodstable.cpp timeperiodstable.hpp
  zonestable.cpp zonestable.hpp
)

if(ICINGA2_UNITY_BUILD)
  mkunity_target(livestatus livestatus livestatus_SOURCES)
endif()

add_library(livestatus OBJECT ${livestatus_SOURCES})

add_dependencies(livestatus base config icinga remote)

set_target_properties (
  livestatus PROPERTIES
  FOLDER Components
)

install_if_not_exists(
  ${PROJECT_SOURCE_DIR}/etc/icinga2/features-available/livestatus.conf
  ${ICINGA2_CONFIGDIR}/features-available
)

install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${ICINGA2_FULL_INITRUNDIR}/cmd\")")

set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}" PARENT_SCOPE)
