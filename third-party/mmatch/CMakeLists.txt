# Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+

set(mmatch_SOURCES
  mmatch.c mmatch.h
)

add_library(mmatch OBJECT ${mmatch_SOURCES})

if(NOT MSVC)
  target_compile_options(mmatch
    PRIVATE
    -Wno-implicit-fallthrough
  )
endif()

set_target_properties(
  mmatch PROPERTIES
  FOLDER Lib
)
