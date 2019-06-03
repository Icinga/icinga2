# Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+

set(execvpe_SOURCES
  execvpe.c execvpe.h
)

add_library(execvpe OBJECT ${execvpe_SOURCES})

set_target_properties (
  execvpe PROPERTIES
  FOLDER Lib
)
