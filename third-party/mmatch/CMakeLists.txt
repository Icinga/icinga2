# Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+

set(mmatch_SOURCES
  mmatch.c mmatch.h
)

add_library(mmatch OBJECT ${mmatch_SOURCES})

set_target_properties(
  mmatch PROPERTIES
  FOLDER Lib
)
