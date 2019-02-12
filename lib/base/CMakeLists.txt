# Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+

mkclass_target(application.ti application-ti.cpp application-ti.hpp)
mkclass_target(configobject.ti configobject-ti.cpp configobject-ti.hpp)
mkclass_target(configuration.ti configuration-ti.cpp configuration-ti.hpp)
mkclass_target(datetime.ti datetime-ti.cpp datetime-ti.hpp)
mkclass_target(filelogger.ti filelogger-ti.cpp filelogger-ti.hpp)
mkclass_target(function.ti function-ti.cpp function-ti.hpp)
mkclass_target(logger.ti logger-ti.cpp logger-ti.hpp)
mkclass_target(perfdatavalue.ti perfdatavalue-ti.cpp perfdatavalue-ti.hpp)
mkclass_target(streamlogger.ti streamlogger-ti.cpp streamlogger-ti.hpp)
mkclass_target(sysloglogger.ti sysloglogger-ti.cpp sysloglogger-ti.hpp)

set(base_SOURCES
  i2-base.hpp
  application.cpp application.hpp application-ti.hpp application-version.cpp application-environment.cpp
  array.cpp array.hpp array-script.cpp
  base64.cpp base64.hpp
  boolean.cpp boolean.hpp boolean-script.cpp
  configobject.cpp configobject.hpp configobject-ti.hpp configobject-script.cpp
  configtype.cpp configtype.hpp
  configuration.cpp configuration.hpp configuration-ti.hpp
  configwriter.cpp configwriter.hpp
  console.cpp console.hpp
  context.cpp context.hpp
  convert.cpp convert.hpp
  datetime.cpp datetime.hpp datetime-ti.hpp datetime-script.cpp
  debug.hpp
  debuginfo.cpp debuginfo.hpp
  dependencygraph.cpp dependencygraph.hpp
  dictionary.cpp dictionary.hpp dictionary-script.cpp
  exception.cpp exception.hpp
  fifo.cpp fifo.hpp
  filelogger.cpp filelogger.hpp filelogger-ti.hpp
  function.cpp function.hpp function-ti.hpp function-script.cpp functionwrapper.hpp
  initialize.cpp initialize.hpp
  json.cpp json.hpp json-script.cpp
  lazy-init.hpp
  library.cpp library.hpp
  loader.cpp loader.hpp
  logger.cpp logger.hpp logger-ti.hpp
  math-script.cpp
  netstring.cpp netstring.hpp
  networkstream.cpp networkstream.hpp
  namespace.cpp namespace.hpp namespace-script.cpp
  number.cpp number.hpp number-script.cpp
  object.cpp object.hpp object-script.cpp
  objectlock.cpp objectlock.hpp
  object-packer.cpp object-packer.hpp
  objecttype.cpp objecttype.hpp
  perfdatavalue.cpp perfdatavalue.hpp perfdatavalue-ti.hpp
  primitivetype.cpp primitivetype.hpp
  process.cpp process.hpp
  reference.cpp reference.hpp reference-script.cpp
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
  streamlogger.cpp streamlogger.hpp streamlogger-ti.hpp
  string.cpp string.hpp string-script.cpp
  stringbuilder.cpp stringbuilder.hpp
  sysloglogger.cpp sysloglogger.hpp sysloglogger-ti.hpp
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

if(HAVE_SYSTEMD)
  find_path(SYSTEMD_INCLUDE_DIR
    NAMES systemd/sd-daemon.h
    HINTS ${SYSTEMD_ROOT_DIR})
  include_directories(${SYSTEMD_INCLUDE_DIR})
endif()

add_library(base OBJECT ${base_SOURCES})

include_directories(${icinga2_SOURCE_DIR}/third-party/execvpe)
link_directories(${icinga2_BINARY_DIR}/third-party/execvpe)

include_directories(${icinga2_SOURCE_DIR}/third-party/mmatch)
link_directories(${icinga2_BINARY_DIR}/third-party/mmatch)

include_directories(${icinga2_SOURCE_DIR}/third-party/socketpair)
link_directories(${icinga2_BINARY_DIR}/third-party/socketpair)

set_target_properties (
  base PROPERTIES
  FOLDER Lib
)

install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${ICINGA2_FULL_CACHEDIR}\")")
install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${ICINGA2_FULL_LOGDIR}/crash\")")

set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}" PARENT_SCOPE)
