# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

add_subdirectory(plugins-contrib.d)

install(
  FILES itl command-icinga.conf hangman plugins command-plugins.conf manubulon command-plugins-manubulon.conf windows-plugins command-plugins-windows.conf nscp command-nscp-local.conf plugins-contrib
  DESTINATION ${ICINGA2_INCLUDEDIR}
)
