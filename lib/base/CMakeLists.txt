# Icinga 2
# Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)
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

mkclass_target(application.ti application.thpp)
mkclass_target(dynamicobject.ti dynamicobject.thpp)
mkclass_target(filelogger.ti filelogger.thpp)
mkclass_target(logger.ti logger.thpp)
mkclass_target(streamlogger.ti streamlogger.thpp)
mkclass_target(sysloglogger.ti sysloglogger.thpp)

set(base_SOURCES
  application.cpp application.thpp array.cpp configerror.cpp console.cpp context.cpp
  convert.cpp debuginfo.cpp dictionary.cpp dynamicobject.cpp dynamicobject.thpp dynamictype.cpp
  exception.cpp fifo.cpp filelogger.cpp filelogger.thpp initialize.cpp json.cpp logger.cpp logger.thpp
  netstring.cpp networkstream.cpp object.cpp primitivetype.cpp process.cpp
  ringbuffer.cpp scriptfunction.cpp scriptfunctionwrapper.cpp
  scriptutils.cpp scriptvariable.cpp serializer.cpp socket.cpp stacktrace.cpp
  statsfunction.cpp stdiostream.cpp stream.cpp streamlogger.cpp streamlogger.thpp string.cpp 
  sysloglogger.cpp sysloglogger.thpp tcpsocket.cpp thinmutex.cpp threadpool.cpp timer.cpp
  tlsstream.cpp tlsutility.cpp type.cpp unixsocket.cpp utility.cpp value.cpp
  value-operators.cpp workqueue.cpp
)

if(ICINGA2_UNITY_BUILD)
    mkunity_target(base base_SOURCES)
endif()

add_library(base SHARED ${base_SOURCES})

target_link_libraries(base ${CMAKE_DL_LIBS} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${YAJL_LIBRARIES} mmatch)

if(HAVE_LIBEXECINFO)
    target_link_libraries(base execinfo)
endif()

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
