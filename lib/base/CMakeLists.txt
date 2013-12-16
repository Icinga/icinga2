# Icinga 2
# Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)
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

mkclass_target(application.ti application.th)
mkclass_target(dynamicobject.ti dynamicobject.th)
mkclass_target(filelogger.ti filelogger.th)
mkclass_target(logger.ti logger.th)
mkclass_target(script.ti script.th)
mkclass_target(streamlogger.ti streamlogger.th)
mkclass_target(sysloglogger.ti sysloglogger.th)

add_library(base SHARED
  application.cpp application.th array.cpp bufferedstream.cpp context.cpp
  convert.cpp dictionary.cpp dynamicobject.cpp dynamicobject.th dynamictype.cpp
  exception.cpp fifo.cpp filelogger.cpp filelogger.th logger.cpp logger.th
  netstring.cpp networkstream.cpp object.cpp objectlock.cpp process.cpp
  process-unix.cpp process-windows.cpp qstring.cpp ringbuffer.cpp script.cpp
  script.th scriptfunction.cpp scriptfunctionwrapper.cpp scriptinterpreter.cpp
  scriptlanguage.cpp scriptvariable.cpp serializer.cpp socket.cpp stacktrace.cpp
  stdiostream.cpp stream_bio.cpp stream.cpp streamlogger.cpp streamlogger.th
  sysloglogger.cpp sysloglogger.th tcpsocket.cpp threadpool.cpp timer.cpp
  tlsstream.cpp tlsutility.cpp type.cpp unixsocket.cpp utility.cpp value.cpp
  workqueue.cpp zlibstream.cpp
)

target_link_libraries(base ${CMAKE_DL_LIBS} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} cJSON mmatch)

include_directories(${icinga2_SOURCE_DIR}/third-party/cJSON)
link_directories(${icinga2_BINARY_DIR}/third-party/cJSON)

include_directories(${icinga2_SOURCE_DIR}/third-party/execvpe)
link_directories(${icinga2_BINARY_DIR}/third-party/execvpe)

include_directories(${icinga2_SOURCE_DIR}/third-party/mmatch)
link_directories(${icinga2_BINARY_DIR}/third-party/mmatch)

if(UNIX OR CYGWIN)
  target_link_libraries(base execvpe)
endif()

if(WIN32)
  target_link_libraries(base ws2_32 dbghelp shlwapi)
endif()

set_target_properties (
  base PROPERTIES
  INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/icinga2
  DEFINE_SYMBOL I2_BASE_BUILD
  FOLDER Lib
)

install(
  TARGETS base
  RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/icinga2
)
