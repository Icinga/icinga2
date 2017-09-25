# Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+

add_subdirectory(mmatch)

if(UNIX OR CYGWIN)
  add_subdirectory(execvpe)
endif()

if(ICINGA2_WITH_REDIS)
  add_subdirectory(hiredis)
endif()

add_subdirectory(socketpair)
