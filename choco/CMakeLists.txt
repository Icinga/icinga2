# Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+

if(WIN32)
  configure_file(icinga2.nuspec.cmake icinga2.nuspec)
  configure_file(chocolateyInstall.ps1.template.cmake chocolateyInstall.ps1.template)
endif()
