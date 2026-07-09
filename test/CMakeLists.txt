# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

include(DiscoverBoostTests)

add_library(testdeps OBJECT
  test-runner.cpp
  test-ctest.cpp
)

if(NOT Boost_UNIT_TEST_FRAMEWORK_LIBRARY)
  find_package(Boost QUIET COMPONENTS unit_test_framework)
endif()

if(Boost_UNIT_TEST_FRAMEWORK_LIBRARY)
  target_link_libraries(testdeps PUBLIC
    Boost::unit_test_framework
  )

  if(NOT Boost_USE_STATIC_LIBS)
    set(BOOST_TEST_DYN_LINK ON)
  endif()
else()
  set(BOOST_TEST_USE_INCLUDED ON)
endif()

configure_file(BoostTestConfig.h.in BoostTestTargetConfig.h @ONLY)

target_include_directories(testdeps
  PUBLIC
  "${CMAKE_CURRENT_BINARY_DIR}"
  # TODO: This is ugly, but needed for "test/*.hpp" includes
  "${CMAKE_SOURCE_DIR}"
)

set_target_properties(testdeps PROPERTIES
  UNITY_BUILD False
)

# In order to test the order of all Icinga 2 config type load dependencies, we need to link against all the libraries,
# but this results in boost signals e.g. in dbevents.cpp being triggered by icinga-checkresult.cpp test cases that
# only pass partially initialised objects. Therefore, the types test cases are decoupled from base and moved to a
# separate executable to not crash the base test cases.
add_executable(testtypes
  icingaapplication-fixture.cpp
  base-type.cpp
)

target_link_libraries(testtypes PUBLIC
  testdeps
  base
  config
  icinga
  remote
)

if(ICINGA2_WITH_CHECKER)
  target_link_libraries(testtypes PUBLIC checker)
endif()

if(ICINGA2_WITH_MYSQL)
  target_link_libraries(testtypes PUBLIC db_ido db_ido_mysql)
endif()

if(ICINGA2_WITH_PGSQL)
  target_link_libraries(testtypes PUBLIC db_ido db_ido_pgsql)
endif()

if(ICINGA2_WITH_ICINGADB)
  target_link_libraries(testtypes PUBLIC icingadb)
endif()

if(ICINGA2_WITH_NOTIFICATION)
  target_link_libraries(testtypes PUBLIC notification)
endif()

if(ICINGA2_WITH_OPENTELEMETRY)
  target_link_libraries(testtypes PUBLIC otel)
endif()

if(ICINGA2_WITH_PERFDATA)
  target_link_libraries(testtypes PUBLIC perfdata)
endif()

target_discover_boost_tests(testtypes)

add_executable(testbase
  icingaapplication-fixture.cpp
  utils.cpp
  base-array.cpp
  base-atomic.cpp
  base-base64.cpp
  base-convert.cpp
  base-dictionary.cpp
  base-fifo.cpp
  base-io-engine.cpp
  base-json.cpp
  base-match.cpp
  base-netstring.cpp
  base-object.cpp
  base-object-packer.cpp
  base-serialize.cpp
  base-shellescape.cpp
  base-stacktrace.cpp
  base-stream.cpp
  base-string.cpp
  base-timer.cpp
  base-tlsutility.cpp base-tlsutility.hpp
  base-utility.cpp
  base-value.cpp
  config-apply.cpp
  config-ops.cpp
  icinga-checkresult.cpp
  icinga-dependencies.cpp
  icinga-legacytimeperiod.cpp
  icinga-macros.cpp
  icinga-notification.cpp
  icinga-perfdata.cpp
  methods-pluginnotificationtask.cpp
  remote-certificate-fixture.cpp
  remote-filterutility.cpp
  remote-configpackageutility.cpp
  remote-httpserverconnection.cpp
  remote-httpmessage.cpp
  remote-httputility.cpp
  remote-url.cpp
)

target_link_libraries(testbase PUBLIC
  testdeps
  base
  config
  icinga
  methods
  remote
)

if(ICINGA2_WITH_NOTIFICATION)
  target_sources(testbase PRIVATE notification-notificationcomponent.cpp)
  target_link_libraries(testbase PUBLIC notification)
endif()

if(ICINGA2_WITH_OPENTELEMETRY)
  target_link_libraries(testbase PUBLIC otel)
endif()

if(ICINGA2_WITH_PERFDATA)
  target_sources(testbase PRIVATE
    perfdata-elasticsearchwriter.cpp
    perfdata-gelfwriter.cpp
    perfdata-graphitewriter.cpp
    perfdata-influxdbwriter.cpp
    perfdata-opentsdbwriter.cpp
    perfdata-perfdatawriterconnection.cpp
  )
  target_link_libraries(testbase PUBLIC perfdata)
endif()

target_discover_boost_tests(testbase)

if(ICINGA2_WITH_LIVESTATUS)
  add_executable(testlivestatus
    icingaapplication-fixture.cpp
    livestatus-fixture.cpp
    livestatus.cpp
  )

  target_link_libraries(testlivestatus PUBLIC
    testdeps
    base
    config
    remote
    icinga
    livestatus
    methods
  )

  target_discover_boost_tests(testlivestatus)
endif()

add_executable(icinga_checkable
  icingaapplication-fixture.cpp
  icinga-checkable-fixture.cpp
  icinga-checkable-flapping.cpp
)

target_link_libraries(icinga_checkable PUBLIC
  testdeps
  base
  config
  remote
  icinga
  cli
  Boost::program_options
)

target_discover_boost_tests(icinga_checkable)
