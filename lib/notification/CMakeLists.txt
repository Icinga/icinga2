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

mkclass_target(notificationcomponent.ti notificationcomponent.thpp)

mkembedconfig_target(notification-type.conf notification-type.cpp)

set(notification_SOURCES
  notificationcomponent.cpp notificationcomponent.thpp notification-type.cpp
)

if(ICINGA2_UNITY_BUILD)
    mkunity_target(notification notification_SOURCES)
endif()

add_library(notification SHARED ${notification_SOURCES})

target_link_libraries(notification ${Boost_LIBRARIES} base config icinga)

set_target_properties (
  notification PROPERTIES
  INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/icinga2
  FOLDER Components
)

install(TARGETS notification RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/icinga2)
