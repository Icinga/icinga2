# Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+

file(GLOB DOCSRCS "*.md")

if(UNIX OR CYGWIN)
  install(
    FILES icinga2.8
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man8
  )
endif()

install(
  FILES ${DOCSRCS}
  DESTINATION ${CMAKE_INSTALL_DOCDIR}/markdown
)

find_program(OPTIPNG_EXECUTABLE optipng)

if(OPTIPNG_EXECUTABLE MATCHES "-NOTFOUND$")
  message(WARNING "optipng not found in \$PATH. Consider installing it.")

  install(
    DIRECTORY images
    DESTINATION "${CMAKE_INSTALL_DOCDIR}/markdown"
  )
else()
  string(LENGTH "${CMAKE_CURRENT_SOURCE_DIR}/" CMAKE_CURRENT_SOURCE_DIR_LENGTH)
  file(GLOB_RECURSE DOCIMGS "*.png")

  foreach(IMG ${DOCIMGS})
    string(LENGTH "${IMG}" IMG_LENGTH)
    math(EXPR LENGTH_DIFF "${IMG_LENGTH} - ${CMAKE_CURRENT_SOURCE_DIR_LENGTH}")
    string(SUBSTRING "${IMG}" ${CMAKE_CURRENT_SOURCE_DIR_LENGTH} ${LENGTH_DIFF} IMG_RELATIVE)
    string(REPLACE "/" "" IMG_RELATIVE_TARGET "${IMG_RELATIVE}")

    add_custom_command(
      OUTPUT "${IMG_RELATIVE}"
      COMMAND "${OPTIPNG_EXECUTABLE}" "-backup" "-out" "${IMG_RELATIVE}" "${IMG}"
      DEPENDS "${IMG}"
      VERBATIM
    )

    add_custom_target("${IMG_RELATIVE_TARGET}" ALL DEPENDS "${IMG_RELATIVE}")

    install(
      FILES "${CMAKE_CURRENT_BINARY_DIR}/${IMG_RELATIVE}"
      DESTINATION "${CMAKE_INSTALL_DOCDIR}/markdown/${IMG_RELATIVE}"
    )
  endforeach()
endif()
