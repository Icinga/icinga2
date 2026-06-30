# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

if(WIN32)
  configure_file(icinga2.nuspec.cmake icinga2.nuspec)
  configure_file(chocolateyInstall.ps1.template.cmake chocolateyInstall.ps1.template)
endif()
