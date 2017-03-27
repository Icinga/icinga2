# Icinga 2
# Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)
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

mkclass_target(rediswriter.ti rediswriter.tcpp rediswriter.thpp)

set(redis_SOURCES
  rediswriter.cpp rediswriter-status.cpp rediswriter-utility.cpp rediswriter.thpp
)

if(ICINGA2_UNITY_BUILD)
    mkunity_target(redis redis redis_SOURCES)
endif()

add_library(redis SHARED ${redis_SOURCES})

target_link_libraries(redis ${Boost_LIBRARIES} base config icinga remote hiredis)

include_directories(${icinga2_SOURCE_DIR}/third-party)
link_directories(${icinga2_BINARY_DIR}/third-party/hiredis)

set_target_properties (
  redis PROPERTIES
  INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/icinga2
  DEFINE_SYMBOL I2_REDIS_BUILD
  FOLDER Components
  VERSION ${SPEC_VERSION}
)

install_if_not_exists(
  ${PROJECT_SOURCE_DIR}/etc/icinga2/features-available/redis.conf
  ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/features-available
)

install(
  TARGETS redis
  RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/icinga2
)

