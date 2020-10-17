# Icinga 2 | (c) 2019 Icinga GmbH | GPLv2+

if(NOT WIN32)
  find_program(GO go)

  add_subdirectory(procmgr)
endif()
