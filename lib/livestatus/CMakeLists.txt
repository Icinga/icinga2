# Icinga 2
# Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)
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

mkclass_target(livestatuslistener.ti livestatuslistener.thpp)

mkembedconfig_target(livestatus-type.conf livestatus-type.cpp)

set(livestatus_SOURCES
  aggregator.cpp andfilter.cpp attributefilter.cpp
  avgaggregator.cpp column.cpp combinerfilter.cpp commandstable.cpp
  commentstable.cpp contactgroupstable.cpp contactstable.cpp countaggregator.cpp
  downtimestable.cpp endpointstable.cpp filter.cpp historytable.cpp
  hostgroupstable.cpp hoststable.cpp invavgaggregator.cpp invsumaggregator.cpp
  livestatuslistener.cpp livestatuslistener.thpp livestatusquery.cpp
  livestatuslogutility.cpp logtable.cpp maxaggregator.cpp
  minaggregator.cpp negatefilter.cpp orfilter.cpp
  servicegroupstable.cpp servicestable.cpp statehisttable.cpp
  statustable.cpp stdaggregator.cpp sumaggregator.cpp table.cpp
  timeperiodstable.cpp livestatus-type.cpp
)

if(ICINGA2_UNITY_BUILD)
    mkunity_target(livestatus livestatus_SOURCES)
endif()

add_library(livestatus SHARED ${livestatus_SOURCES})

target_link_libraries(livestatus ${Boost_LIBRARIES} base config icinga remote)

set_target_properties (
  livestatus PROPERTIES
  INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/icinga2
  FOLDER Components
)

install_if_not_exists(
  ${PROJECT_SOURCE_DIR}/etc/icinga2/features-available/livestatus.conf
  ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/features-available
)

install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${ICINGA2_RUNDIR}/icinga2/cmd\")")

install(TARGETS livestatus RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/icinga2)

