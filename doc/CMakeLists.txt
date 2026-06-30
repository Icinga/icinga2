# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

file(GLOB DOCSRCS "*.md")

if(UNIX OR CYGWIN)
  install(
    FILES icinga2.8
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man8
  )
endif()

install(
  FILES ${DOCSRCS}
  DESTINATION ${CMAKE_INSTALL_DOCDIR}/markdown
)

install(
  DIRECTORY images
  DESTINATION ${CMAKE_INSTALL_DOCDIR}/markdown
)
