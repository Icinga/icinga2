# Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+

set(socketpair_SOURCES
  socketpair.c socketpair.h
)

add_library(socketpair OBJECT ${socketpair_SOURCES})

set_target_properties (
  socketpair PROPERTIES
  FOLDER Lib
)
