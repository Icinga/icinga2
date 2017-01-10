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

add_library(mmatch SHARED mmatch.c mmatch.h)

set_target_properties(
  mmatch PROPERTIES
  DEFINE_SYMBOL I2_MMATCH_BUILD
  FOLDER Lib
  VERSION ${SPEC_VERSION}
)

install(
  TARGETS mmatch
  RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/icinga2
)

if(APPLE)
  install(
    TARGETS mmatch
    LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}/icinga-studio.app/Contents
  )
endif()
