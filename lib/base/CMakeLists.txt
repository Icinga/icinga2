# Icinga 2
# Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)
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
mkclass_target(perfdatavalue.ti perfdatavalue.tcpp perfdatavalue.thpp)
mkclass_target(streamlogger.ti streamlogger.tcpp streamlogger.thpp)
mkclass_target(sysloglogger.ti sysloglogger.tcpp sysloglogger.thpp)

set(base_SOURCES
  i2-base.hpp
  application.cpp application.hpp application.thpp application-version.cpp
  array.cpp array.hpp array-script.cpp
  base64.cpp base64.hpp
  boolean.cpp boolean.hpp boolean-script.cpp
  configobject.cpp configobject.hpp configobject.thpp configobject-script.cpp
  configtype.cpp configtype.hpp
  configwriter.cpp configwriter.hpp
  console.cpp console.hpp
  context.cpp context.hpp
  convert.cpp convert.hpp
  datetime.cpp datetime.hpp datetime.thpp datetime-script.cpp
  debug.hpp
  debuginfo.cpp debuginfo.hpp
  dependencygraph.cpp dependencygraph.hpp
  dictionary.cpp dictionary.hpp dictionary-script.cpp
  exception.cpp exception.hpp
  fifo.cpp fifo.hpp
  filelogger.cpp filelogger.hpp filelogger.thpp
  function.cpp function.hpp function.thpp function-script.cpp functionwrapper.hpp
  initialize.cpp initialize.hpp
  json.cpp json.hpp json-script.cpp
  library.cpp library.hpp
  loader.cpp loader.hpp
  logger.cpp logger.hpp logger.thpp
  math-script.cpp
  netstring.cpp netstring.hpp
  networkstream.cpp networkstream.hpp
  number.cpp number.hpp number-script.cpp
  object.cpp object.hpp object-script.cpp
  objectlock.cpp objectlock.hpp
  objecttype.cpp objecttype.hpp
  perfdatavalue.cpp perfdatavalue.hpp perfdatavalue.thpp
  primitivetype.cpp primitivetype.hpp
  process.cpp process.hpp
  registry.hpp
  ringbuffer.cpp ringbuffer.hpp
  scriptframe.cpp scriptframe.hpp
  scriptglobal.cpp scriptglobal.hpp
  scriptutils.cpp scriptutils.hpp
  serializer.cpp serializer.hpp
  singleton.hpp
  socket.cpp socket.hpp
  socketevents.cpp socketevents-epoll.cpp socketevents-poll.cpp socketevents.hpp
  stacktrace.cpp stacktrace.hpp
  statsfunction.hpp
  stdiostream.cpp stdiostream.hpp
  stream.cpp stream.hpp
  streamlogger.cpp streamlogger.hpp streamlogger.thpp
  string.cpp string.hpp string-script.cpp
  sysloglogger.cpp sysloglogger.hpp sysloglogger.thpp
  tcpsocket.cpp tcpsocket.hpp
  threadpool.cpp threadpool.hpp
  timer.cpp timer.hpp
  tlsstream.cpp tlsstream.hpp
  tlsutility.cpp tlsutility.hpp
  type.cpp type.hpp typetype-script.cpp
  unix.hpp
  unixsocket.cpp unixsocket.hpp
  utility.cpp utility.hpp
  value.cpp value.hpp value-operators.cpp
  win32.hpp
  workqueue.cpp workqueue.hpp
)

set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/application-version.cpp PROPERTY EXCLUDE_UNITY_BUILD TRUE)

if(ICINGA2_UNITY_BUILD)
    mkunity_target(base base base_SOURCES)
endif()

add_library(base STATIC ${base_SOURCES})

target_link_libraries(base PUBLIC ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY})
target_link_libraries(base PRIVATE ${Boost_REGEX_LIBRARY} ${CMAKE_DL_LIBS} ${OPENSSL_LIBRARIES} ${YAJL_LIBRARIES} mmatch socketpair)

if(HAVE_LIBEXECINFO)
    target_link_libraries(base PRIVATE execinfo)
endif()

include_directories(${icinga2_SOURCE_DIR}/third-party/execvpe)
link_directories(${icinga2_BINARY_DIR}/third-party/execvpe)

include_directories(${icinga2_SOURCE_DIR}/third-party/mmatch)
link_directories(${icinga2_BINARY_DIR}/third-party/mmatch)

include_directories(${icinga2_SOURCE_DIR}/third-party/socketpair)
link_directories(${icinga2_BINARY_DIR}/third-party/socketpair)

if(UNIX OR CYGWIN)
  target_link_libraries(base PRIVATE execvpe)
endif()

if(WIN32)
  target_link_libraries(base PRIVATE ws2_32 dbghelp shlwapi msi)
endif()

set_target_properties(
  base PROPERTIES
  FOLDER Lib
)

if(ICINGA2_PCH_BUILD)
  target_precompiled_header(base pch.hpp)
endif()

install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/cache/icinga2\")")
install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/log/icinga2/crash\")")

set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}" PARENT_SCOPE)
