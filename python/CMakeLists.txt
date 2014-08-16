# Icinga 2
# Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)
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

find_package(PythonInterp REQUIRED)

add_subdirectory(icinga2)

configure_file(setup.py.cmake ${CMAKE_CURRENT_BINARY_DIR}/setup.py @ONLY)

install(CODE "
  execute_process(
   COMMAND ${CMAKE_COMMAND}
     -DSETUP_PY=${CMAKE_CURRENT_BINARY_DIR}/setup.py
     -DPYTHON_EXECUTABLE=${PYTHON_EXECUTABLE}
     -DPREFIX=${CMAKE_INSTALL_PREFIX}
     -DWDIR=${CMAKE_CURRENT_BINARY_DIR}
     -P ${PROJECT_SOURCE_DIR}/third-party/cmake/PythonSetup.cmake
  )

  configure_file(
      \$ENV{DESTDIR}${CMAKE_INSTALL_FULL_BINDIR}/icinga2-list-objects
      \$ENV{DESTDIR}${CMAKE_INSTALL_FULL_SBINDIR}/icinga2-list-objects COPYONLY}
  )

  file(
    REMOVE
      \$ENV{DESTDIR}${CMAKE_INSTALL_FULL_BINDIR}/icinga2-list-objects
  )"
)
