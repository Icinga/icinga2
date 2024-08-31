find_package(Doxygen REQUIRED)
find_package(Sphinx REQUIRED)

set(DOXYGEN_INPUT_DIR ${PROJECT_SOURCE_DIR}/include/boost/wintls/)
set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/doxygen)
set(DOXYGEN_INDEX_FILE ${DOXYGEN_OUTPUT_DIR}/index.xml)
set(DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
set(DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

set(WINTLS_PUBLIC_HEADERS
  ${DOXYGEN_INPUT_DIR}/certificate.hpp
  ${DOXYGEN_INPUT_DIR}/context.hpp
  ${DOXYGEN_INPUT_DIR}/file_format.hpp
  ${DOXYGEN_INPUT_DIR}/handshake_type.hpp
  ${DOXYGEN_INPUT_DIR}/method.hpp
  ${DOXYGEN_INPUT_DIR}/stream.hpp
)

string(REPLACE ";" " " DOXYGEN_INPUT_FILES "${WINTLS_PUBLIC_HEADERS}")

configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

add_custom_command(OUTPUT ${DOXYGEN_INDEX_FILE}
		DEPENDS ${WINTLS_PUBLIC_HEADERS}
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
		MAIN_DEPENDENCY ${DOXYFILE_OUT} ${DOXYFILE_IN}
    COMMENT "Generating docs"
		VERBATIM)

add_custom_target(Doxygen ALL DEPENDS ${DOXYGEN_INDEX_FILE})

set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR})
set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/html)
set(SPHINX_INDEX_FILE ${SPHINX_BUILD}/index.html)
set(SPHINX_SOURCE_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/API.rst
  ${CMAKE_CURRENT_SOURCE_DIR}/async_https_client.rst
  ${CMAKE_CURRENT_SOURCE_DIR}/classes.rst
  ${CMAKE_CURRENT_SOURCE_DIR}/echo_client.rst
  ${CMAKE_CURRENT_SOURCE_DIR}/echo_server.rst
  ${CMAKE_CURRENT_SOURCE_DIR}/enumerations.rst
  ${CMAKE_CURRENT_SOURCE_DIR}/examples.rst
  ${CMAKE_CURRENT_SOURCE_DIR}/functions.rst
  ${CMAKE_CURRENT_SOURCE_DIR}/https_client.rst
  ${CMAKE_CURRENT_SOURCE_DIR}/index.rst
  ${CMAKE_CURRENT_SOURCE_DIR}/type_aliases.rst
  ${CMAKE_CURRENT_SOURCE_DIR}/usage.rst
)

add_custom_command(OUTPUT ${SPHINX_INDEX_FILE}
	COMMAND
		${SPHINX_EXECUTABLE} -b html
		-D breathe_projects.boost-wintls=${DOXYGEN_OUTPUT_DIR}
    -D version=${PROJECT_VERSION}
		${SPHINX_SOURCE} ${SPHINX_BUILD}
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/static/boost-wintls.css
    ${CMAKE_CURRENT_SOURCE_DIR}/templates/layout.html
    ${CMAKE_CURRENT_SOURCE_DIR}/templates/navbar.html
    ${CMAKE_CURRENT_SOURCE_DIR}/extensions/toctree_elements.py
    ${CMAKE_CURRENT_SOURCE_DIR}/extensions/remove_inline_specifier.py
    ${SPHINX_SOURCE_FILES}
		${DOXYGEN_INDEX_FILE}
	MAIN_DEPENDENCY ${SPHINX_SOURCE}/conf.py
	COMMENT "Generating documentation with Sphinx"
)

add_custom_target(doc ALL DEPENDS ${SPHINX_INDEX_FILE})
