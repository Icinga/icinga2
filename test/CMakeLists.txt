# Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+

include(DiscoverBoostTests)

add_library(testdeps INTERFACE)

if(NOT Boost_UNIT_TEST_FRAMEWORK_LIBRARY)
  find_package(Boost QUIET COMPONENTS unit_test_framework)
endif()

if(Boost_UNIT_TEST_FRAMEWORK_LIBRARY)
  target_link_libraries(testdeps
    INTERFACE
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
  )
  if(NOT Boost_USE_STATIC_LIBS)
    set(BOOST_TEST_DYN_LINK ON)
  endif()
else()
  set(BOOST_TEST_USE_INCLUDED ON)
endif()

target_link_libraries(testdeps
  INTERFACE
  ${base_DEPS}
)

configure_file(BoostTestConfig.h.in BoostTestTargetConfig.h @ONLY)

target_include_directories(testdeps
  INTERFACE
  "${CMAKE_CURRENT_BINARY_DIR}"
)

target_sources(testdeps INTERFACE
  test-runner.cpp
  test-ctest.cpp
)

Set(types_test_SOURCES
  icingaapplication-fixture.cpp
  base-type.cpp
  ${base_OBJS}
  $<TARGET_OBJECTS:config>
  $<TARGET_OBJECTS:remote>
  $<TARGET_OBJECTS:icinga>
  $<TARGET_OBJECTS:methods>
)

if(ICINGA2_WITH_CHECKER)
  list(APPEND types_test_SOURCES $<TARGET_OBJECTS:checker>)
endif()

if(ICINGA2_WITH_MYSQL)
  list(APPEND types_test_SOURCES $<TARGET_OBJECTS:db_ido> $<TARGET_OBJECTS:db_ido_mysql>)
endif()

if(ICINGA2_WITH_PGSQL)
  list(APPEND types_test_SOURCES $<TARGET_OBJECTS:db_ido> $<TARGET_OBJECTS:db_ido_pgsql>)
endif()

if(ICINGA2_WITH_ICINGADB)
  list(APPEND types_test_SOURCES $<TARGET_OBJECTS:icingadb>)
endif()

if(ICINGA2_WITH_NOTIFICATION)
  list(APPEND types_test_SOURCES $<TARGET_OBJECTS:notification>)
endif()

if(ICINGA2_WITH_PERFDATA)
  list(APPEND types_test_SOURCES $<TARGET_OBJECTS:perfdata>)
endif()

if(ICINGA2_UNITY_BUILD)
  mkunity_target(types test types_test_SOURCES)
endif()

# In order to test the order of all Icinga 2 config type load dependencies, we need to link against all the libraries,
# but this results in boost signals e.g. in dbevents.cpp being triggered by icinga-checkresult.cpp test cases that
# only pass partially initialised objects. Therefore, the types test cases are decoupled from base and moved to a
# separate executable to not crash the base test cases.
add_executable(testtypes
  ${types_test_SOURCES}
)
target_link_libraries(testtypes testdeps)
target_discover_boost_tests(testtypes)

set(base_test_SOURCES
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
  base-tlsutility.cpp
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
  ${base_OBJS}
  $<TARGET_OBJECTS:config>
  $<TARGET_OBJECTS:remote>
  $<TARGET_OBJECTS:icinga>
  $<TARGET_OBJECTS:methods>
)

if(ICINGA2_WITH_NOTIFICATION)
  list(APPEND base_test_SOURCES
    notification-notificationcomponent.cpp
    $<TARGET_OBJECTS:notification>
  )
endif()

if(ICINGA2_UNITY_BUILD)
  mkunity_target(base test base_test_SOURCES)
endif()

add_executable(testbase
  ${base_test_SOURCES}
)
target_link_libraries(testbase testdeps)
target_discover_boost_tests(testbase)

if(ICINGA2_WITH_LIVESTATUS)
  set(livestatus_test_SOURCES
    icingaapplication-fixture.cpp
    livestatus-fixture.cpp
    livestatus.cpp
    ${base_OBJS}
    $<TARGET_OBJECTS:config>
    $<TARGET_OBJECTS:remote>
    $<TARGET_OBJECTS:icinga>
    $<TARGET_OBJECTS:livestatus>
    $<TARGET_OBJECTS:methods>
  )

  if(ICINGA2_UNITY_BUILD)
      mkunity_target(livestatus test livestatus_test_SOURCES)
  endif()

  add_executable(testlivestatus
    ${livestatus_test_SOURCES}
  )

  target_link_libraries(testlivestatus testdeps)
  target_discover_boost_tests(testlivestatus)
endif()

set(icinga_checkable_test_SOURCES
  icingaapplication-fixture.cpp
  icinga-checkable-fixture.cpp
  icinga-checkable-flapping.cpp
  ${base_OBJS}
  $<TARGET_OBJECTS:config>
  $<TARGET_OBJECTS:remote>
  $<TARGET_OBJECTS:icinga>
  $<TARGET_OBJECTS:cli>
)

if(ICINGA2_UNITY_BUILD)
    mkunity_target(icinga_checkable test icinga_checkable_test_SOURCES)
endif()

add_executable(icinga_checkable
  ${icinga_checkable_test_SOURCES}
)
target_link_libraries(icinga_checkable testdeps)
target_discover_boost_tests(icinga_checkable)
