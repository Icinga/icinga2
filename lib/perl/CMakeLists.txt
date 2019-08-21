# Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+

set(perl_SOURCES
  i2-perl.hpp
  languageperl.cpp languageperl.hpp
)

if(ICINGA2_UNITY_BUILD)
  mkunity_target(perl perl perl_SOURCES)
endif()

add_library(perl OBJECT ${perl_SOURCES})

add_dependencies(perl base)

set_target_properties (
  perl PROPERTIES
  FOLDER Lib
)

configure_file(embed.pl.cmake "${CMAKE_CURRENT_BINARY_DIR}/perl/embed.pl" @ONLY)

install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}/${CMAKE_INSTALL_PREFIX}/lib/icinga2/perl\")")

install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/perl/embed.pl"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/icinga2/perl
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)
