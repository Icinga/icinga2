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

add_library(hiredis SHARED net.c net.h hiredis.c hiredis.h sds.c sds.h async.c async.h read.c read.h)

if(HAVE_VISIBILITY_HIDDEN)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=default")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=default")
endif()

set_target_properties (
  hiredis PROPERTIES
  FOLDER Lib
  VERSION ${SPEC_VERSION}
)

install(
  TARGETS hiredis
  RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/icinga2
)

if(APPLE)
  install(
    TARGETS hiredis
    LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}/icinga-studio.app/Contents
  )
endif()
