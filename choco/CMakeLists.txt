# Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+

if(WIN32)
  find_program(CHOCO_BINARY choco)

  configure_file(icinga2.nuspec.cmake icinga2.nuspec)
  configure_file(chocolateyInstall.ps1.cmake chocolateyInstall.ps1)

  add_custom_target(choco-pkg ALL
    COMMAND choco pack
    COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_CURRENT_BINARY_DIR}/icinga2.${ICINGA2_VERSION_SAFE}.nupkg ${CMAKE_CURRENT_BINARY_DIR}/icinga2.nupkg
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/icinga2.nuspec ${CMAKE_CURRENT_BINARY_DIR}/chocolateyInstall.ps1 chocolateyUninstall.ps1
  )
endif()
