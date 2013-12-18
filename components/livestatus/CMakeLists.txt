# Icinga 2
# Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)
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

mkclass_target(listener.ti listener.th)

mkembedconfig_target(livestatus-type.conf livestatus-type.cpp)

add_library(livestatus SHARED aggregator.cpp andfilter.cpp attributefilter.cpp avgaggregator.cpp column.cpp combinerfilter.cpp commandstable.cpp commentstable.cpp contactgroupstable.cpp contactstable.cpp countaggregator.cpp downtimestable.cpp filter.cpp historytable.cpp hostgroupstable.cpp hoststable.cpp invavgaggregator.cpp invsumaggregator.cpp listener.cpp listener.th logutility.cpp logtable.cpp maxaggregator.cpp minaggregator.cpp negatefilter.cpp orfilter.cpp query.cpp servicegroupstable.cpp servicestable.cpp statehisttable.cpp statustable.cpp stdaggregator.cpp sumaggregator.cpp table.cpp timeperiodstable.cpp livestatus-type.cpp)

target_link_libraries(livestatus ${Boost_LIBRARIES} base config icinga)

set_target_properties (
  livestatus PROPERTIES
  INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/icinga2
  FOLDER Components
)

install(TARGETS livestatus RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/icinga2)
