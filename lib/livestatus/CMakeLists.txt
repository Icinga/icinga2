# Icinga 2
# Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)
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

mkclass_target(livestatuslistener.ti livestatuslistener.tcpp livestatuslistener.thpp)

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
  livestatuslistener.cpp livestatuslistener.hpp livestatuslistener.thpp
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

add_library(livestatus STATIC ${livestatus_SOURCES})

target_link_libraries(livestatus ${Boost_LIBRARIES} base config icinga remote)

set_target_properties (
  livestatus PROPERTIES
  FOLDER Components
)

install_if_not_exists(
  ${PROJECT_SOURCE_DIR}/etc/icinga2/features-available/livestatus.conf
  ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/features-available
)

install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${ICINGA2_RUNDIR}/icinga2/cmd\")")

set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}" PARENT_SCOPE)
