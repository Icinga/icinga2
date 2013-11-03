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

if(UNIX OR CYGWIN)
  # TODO: dependencies
  add_custom_command(
    OUTPUT index.html
    COMMAND ./mkdoc.sh
    ARGS > ${CMAKE_CURRENT_BINARY_DIR}/index.html.tmp
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy ${CMAKE_CURRENT_BINARY_DIR}/index.html.tmp ${CMAKE_CURRENT_BINARY_DIR}/index.html
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )

  add_custom_target(index-html ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/index.html)

  install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/index.html jquery.min.js jquery.toc.min.js toc.css toc-margin.css toc-print.css
    DESTINATION ${CMAKE_INSTALL_DOCDIR}/htdocs
  )

  install(
    DIRECTORY strapdownjs
    DESTINATION ${CMAKE_INSTALL_DOCDIR}/htdocs
  )

  install(
    FILES icinga2.8
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man8
  )
endif()
