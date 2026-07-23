# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

add_library(base OBJECT
  i2-base.hpp
  application.cpp application.hpp application.ti
  application-version.cpp application-environment.cpp
  array.cpp array.hpp array-script.cpp
  atomic.hpp
  atomic-file.cpp atomic-file.hpp
  base64.cpp base64.hpp
  boolean.cpp boolean.hpp boolean-script.cpp
  bulker.hpp
  configobject.cpp configobject.hpp configobject.ti
  configobject-script.cpp
  configtype.cpp configtype.hpp
  configuration.cpp configuration.hpp configuration.ti
  configwriter.cpp configwriter.hpp
  console.cpp console.hpp
  context.cpp context.hpp
  convert.cpp convert.hpp
  datetime.cpp datetime.hpp datetime.ti
  datetime-script.cpp
  debug.hpp
  debuginfo.cpp debuginfo.hpp
  dependencygraph.cpp dependencygraph.hpp
  dictionary.cpp dictionary.hpp dictionary-script.cpp
  exception.cpp exception.hpp
  fifo.cpp fifo.hpp
  filelogger.cpp filelogger.hpp filelogger.ti
  function.cpp function.hpp function.ti
  function-script.cpp functionwrapper.hpp
  generator.hpp
  initialize.cpp initialize.hpp
  intrusive-ptr.hpp
  io-engine.cpp io-engine.hpp
  journaldlogger.cpp journaldlogger.hpp journaldlogger.ti
  json.cpp json.hpp json-script.cpp
  lazy-init.hpp
  library.cpp library.hpp
  loader.cpp loader.hpp
  logger.cpp logger.hpp logger.ti
  math-script.cpp
  netstring.cpp netstring.hpp
  networkstream.cpp networkstream.hpp
  namespace.cpp namespace.hpp namespace-script.cpp
  number.cpp number.hpp number-script.cpp
  object.cpp object.hpp object-script.cpp
  objectlock.cpp objectlock.hpp
  object-packer.cpp object-packer.hpp
  objecttype.cpp objecttype.hpp
  perfdatavalue.cpp perfdatavalue.hpp perfdatavalue.ti
  primitivetype.cpp primitivetype.hpp
  process.cpp process.hpp
  reference.cpp reference.hpp reference-script.cpp
  registry.hpp
  ringbuffer.cpp ringbuffer.hpp
  scriptframe.cpp scriptframe.hpp
  scriptglobal.cpp scriptglobal.hpp
  scriptpermission.cpp scriptpermission.hpp
  scriptutils.cpp scriptutils.hpp
  serializer.cpp serializer.hpp
  shared.hpp
  shared-memory.hpp
  shared-object.hpp
  singleton.hpp
  socket.cpp socket.hpp
  stacktrace.cpp stacktrace.hpp
  statsfunction.hpp
  stdiostream.cpp stdiostream.hpp
  stream.cpp stream.hpp
  streamlogger.cpp streamlogger.hpp streamlogger.ti
  string.cpp string.hpp string-script.cpp
  sysloglogger.cpp sysloglogger.hpp sysloglogger.ti
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
  wait-group.cpp wait-group.hpp
  win32.hpp
  workqueue.cpp workqueue.hpp
)

set_source_files_properties(application-version.cpp PROPERTIES
  SKIP_UNITY_BUILD_INCLUSION TRUE
)

if(WIN32)
  target_sources(base PRIVATE
    windowseventloglogger.cpp
    windowseventloglogger.hpp
    windowseventloglogger.ti
  )

  target_link_libraries(base PUBLIC
    ws2_32
    dbghelp
    shlwapi
    msi
  )

  # Generate a DLL containing message definitions for the Windows Event Viewer.
  # See also: https://docs.microsoft.com/en-us/windows/win32/eventlog/reporting-an-event
  add_custom_command(
    OUTPUT windowseventloglogger-provider.rc windowseventloglogger-provider.h
    COMMAND mc ARGS -U ${CMAKE_CURRENT_SOURCE_DIR}/windowseventloglogger-provider.mc
    DEPENDS windowseventloglogger-provider.mc
  )

  target_sources(base PRIVATE
    windowseventloglogger-provider.h
  )

  add_custom_command(
    OUTPUT windowseventloglogger-provider.res
    COMMAND rc ARGS windowseventloglogger-provider.rc
    DEPENDS windowseventloglogger-provider.rc
  )

  add_library(eventprovider MODULE windowseventloglogger-provider.res windowseventloglogger-provider.rc)
  set_target_properties(eventprovider PROPERTIES LINKER_LANGUAGE CXX)
  target_link_libraries(eventprovider PRIVATE -noentry)

  install(TARGETS eventprovider LIBRARY DESTINATION ${CMAKE_INSTALL_SBINDIR})
endif()

target_mkclass(base base_generated config_generated)

target_link_libraries(base PUBLIC
  common
)

if(Execinfo_FOUND)
  target_link_libraries(common PUBLIC Execinfo)
endif()

if(USE_SYSTEMD)
  target_link_libraries(base PUBLIC
    Systemd
  )

  # TODO: Decide if this is any better than setting it in the source file directly.
  set_source_files_properties(journaldlogger.cpp PROPERTIES
    COMPILE_DEFINITIONS
    SD_JOURNAL_SUPPRESS_LOCATION
  )
endif()

set_target_properties (
  base PROPERTIES
  FOLDER Lib
)

if(NOT WIN32)
  install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${ICINGA2_FULL_CACHEDIR}\")")
  install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${ICINGA2_FULL_LOGDIR}/crash\")")
endif()

set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}" PARENT_SCOPE)
