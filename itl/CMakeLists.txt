# Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+

add_subdirectory(plugins-contrib.d)

install(
  FILES itl command-icinga.conf hangman plugins command-plugins.conf manubulon command-plugins-manubulon.conf windows-plugins command-plugins-windows.conf nscp command-nscp-local.conf plugins-contrib
  DESTINATION ${ICINGA2_INCLUDEDIR}
)
