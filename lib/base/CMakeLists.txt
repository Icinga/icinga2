# Icinga 2
# Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation
# Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

mkclass_target(application.ti application.tcpp application.thpp)
mkclass_target(configobject.ti configobject.tcpp configobject.thpp)
mkclass_target(datetime.ti datetime.tcpp datetime.thpp)
mkclass_target(filelogger.ti filelogger.tcpp filelogger.thpp)
mkclass_target(function.ti function.tcpp function.thpp)
mkclass_target(logger.ti logger.tcpp logger.thpp)
mkclass_target(streamlogger.ti streamlogger.tcpp streamlogger.thpp)
mkclass_target(sysloglogger.ti sysloglogger.tcpp sysloglogger.thpp)

set(base_SOURCES
  application.cpp application.thpp application-version.cpp array.cpp
  array-script.cpp boolean.cpp boolean-script.cpp console.cpp context.cpp
  convert.cpp datetime.cpp datetime.thpp datetime-script.cpp debuginfo.cpp dictionary.cpp dictionary-script.cpp
  configobject.cpp configobject.thpp configobject-script.cpp configtype.cpp configwriter.cpp dependencygraph.cpp
  exception.cpp fifo.cpp filelogger.cpp filelogger.thpp initialize.cpp json.cpp
  json-script.cpp loader.cpp logger.cpp logger.thpp math-script.cpp
  netstring.cpp networkstream.cpp number.cpp number-script.cpp object.cpp
  object-script.cpp objecttype.cpp primitivetype.cpp process.cpp ringbuffer.cpp scriptframe.cpp
  function.cpp function.thpp function-script.cpp functionwrapper.cpp scriptglobal.cpp
  scriptutils.cpp serializer.cpp socket.cpp socketevents.cpp socketevents-epoll.cpp socketevents-poll.cpp stacktrace.cpp
  statsfunction.cpp stdiostream.cpp stream.cpp streamlogger.cpp streamlogger.thpp string.cpp string-script.cpp
  sysloglogger.cpp sysloglogger.thpp tcpsocket.cpp threadpool.cpp timer.cpp
  tlsstream.cpp tlsutility.cpp type.cpp typetype-script.cpp unixsocket.cpp utility.cpp value.cpp
  value-operators.cpp workqueue.cpp
)

set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/application-version.cpp PROPERTY EXCLUDE_UNITY_BUILD TRUE)

if(ICINGA2_UNITY_BUILD)
    mkunity_target(base base base_SOURCES)
endif()

add_library(base SHARED ${base_SOURCES})

target_link_libraries(base ${CMAKE_DL_LIBS} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${YAJL_LIBRARIES} mmatch socketpair)

if(HAVE_LIBEXECINFO)
    target_link_libraries(base execinfo)
endif()

include_directories(${icinga2_SOURCE_DIR}/third-party/execvpe)
link_directories(${icinga2_BINARY_DIR}/third-party/execvpe)

include_directories(${icinga2_SOURCE_DIR}/third-party/mmatch)
link_directories(${icinga2_BINARY_DIR}/third-party/mmatch)

include_directories(${icinga2_SOURCE_DIR}/third-party/socketpair)
link_directories(${icinga2_BINARY_DIR}/third-party/socketpair)

if(UNIX OR CYGWIN)
  target_link_libraries(base execvpe)
endif()

if(WIN32)
  target_link_libraries(base ws2_32 dbghelp shlwapi msi)
endif()

set_target_properties (
  base PROPERTIES
  INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/icinga2
  DEFINE_SYMBOL I2_BASE_BUILD
  FOLDER Lib
  VERSION ${SPEC_VERSION}
)

install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/cache/icinga2\")")
install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/log/icinga2/crash\")")

install(
  TARGETS base
  RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/icinga2
)

if(APPLE)
  install(
    TARGETS base
    LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}/icinga-studio.app/Contents
  )
endif()

set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}" PARENT_SCOPE)
