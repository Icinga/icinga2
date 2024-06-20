# Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+

set(crypt_blowfish_SOURCES
  crypt_blowfish.c crypt_blowfish.h
)

add_library(crypt_blowfish OBJECT ${crypt_blowfish_SOURCES})

set_target_properties (
  crypt_blowfish PROPERTIES
  FOLDER Lib
)
