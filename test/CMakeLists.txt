# Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+

include(BoostTestTargets)

set(base_test_SOURCES
  icingaapplication-fixture.cpp
  base-array.cpp
  base-base64.cpp
  base-convert.cpp
  base-dictionary.cpp
  base-fifo.cpp
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
  base-type.cpp
  base-utility.cpp
  base-value.cpp
  config-ops.cpp
  icinga-checkresult.cpp
  icinga-dependencies.cpp
  icinga-legacytimeperiod.cpp
  icinga-macros.cpp
  icinga-notification.cpp
  icinga-perfdata.cpp
  remote-configpackageutility.cpp
  remote-url.cpp
  ${base_OBJS}
  $<TARGET_OBJECTS:config>
  $<TARGET_OBJECTS:remote>
  $<TARGET_OBJECTS:icinga>
)

if(ICINGA2_UNITY_BUILD)
  mkunity_target(base test base_test_SOURCES)
endif()

add_boost_test(base
  SOURCES test-runner.cpp ${base_test_SOURCES}
  LIBRARIES ${base_DEPS}
  TESTS
    base_array/construct
    base_array/getset
    base_array/resize
    base_array/insert
    base_array/remove
    base_array/unique
    base_array/foreach
    base_array/clone
    base_array/json
    base_base64/base64
    base_convert/tolong
    base_convert/todouble
    base_convert/tostring
    base_convert/tobool
    base_dictionary/construct
    base_dictionary/initializer1
    base_dictionary/initializer2
    base_dictionary/get1
    base_dictionary/get2
    base_dictionary/foreach
    base_dictionary/remove
    base_dictionary/clone
    base_dictionary/json
    base_dictionary/keys_ordered
    base_fifo/construct
    base_fifo/io
    base_json/encode
    base_json/decode
    base_json/invalid1
    base_object_packer/pack_null
    base_object_packer/pack_false
    base_object_packer/pack_true
    base_object_packer/pack_number
    base_object_packer/pack_string
    base_object_packer/pack_array
    base_object_packer/pack_object
    base_match/tolong
    base_netstring/netstring
    base_object/construct
    base_object/getself
    base_serialize/scalar
    base_serialize/array
    base_serialize/dictionary
    base_serialize/object
    base_shellescape/escape_basic
    base_shellescape/escape_quoted
    base_stacktrace/stacktrace
    base_stream/readline_stdio
    base_string/construct
    base_string/equal
    base_string/clear
    base_string/append
    base_string/trim
    base_string/contains
    base_string/replace
    base_string/index
    base_string/find
    base_timer/construct
    base_timer/interval
    base_timer/invoke
    base_timer/scope
    base_tlsutility/sha1
    base_type/gettype
    base_type/assign
    base_type/byname
    base_type/instantiate
    base_utility/parse_version
    base_utility/compare_version
    base_utility/comparepasswords_works
    base_utility/comparepasswords_issafe
    base_utility/validateutf8
    base_utility/EscapeCreateProcessArg
    base_utility/TruncateUsingHash
    base_value/scalar
    base_value/convert
    base_value/format
    config_ops/simple
    config_ops/advanced
    icinga_checkresult/host_1attempt
    icinga_checkresult/host_2attempts
    icinga_checkresult/host_3attempts
    icinga_checkresult/service_1attempt
    icinga_checkresult/service_2attempts
    icinga_checkresult/service_3attempts
    icinga_checkresult/host_flapping_notification
    icinga_checkresult/service_flapping_notification
    icinga_dependencies/multi_parent
    icinga_notification/strings
    icinga_notification/state_filter
    icinga_notification/type_filter
    icinga_macros/simple
    icinga_legacytimeperiod/simple
    icinga_legacytimeperiod/advanced
    icinga_legacytimeperiod/dst
    icinga_legacytimeperiod/dst_isinside
    icinga_perfdata/empty
    icinga_perfdata/simple
    icinga_perfdata/quotes
    icinga_perfdata/multiple
    icinga_perfdata/normalize
    icinga_perfdata/uom
    icinga_perfdata/warncritminmax
    icinga_perfdata/ignore_invalid_warn_crit_min_max
    icinga_perfdata/invalid
    icinga_perfdata/multi
    icinga_perfdata/scientificnotation
    icinga_perfdata/parse_edgecases
    remote_configpackageutility/ValidateName
    remote_url/id_and_path
    remote_url/parameters
    remote_url/get_and_set
    remote_url/format
    remote_url/illegal_legal_strings
)

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

  add_boost_test(livestatus
    SOURCES test-runner.cpp ${livestatus_test_SOURCES}
    LIBRARIES ${base_DEPS}
    TESTS livestatus/hosts livestatus/services
  )
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

add_boost_test(icinga_checkable
  SOURCES test-runner.cpp ${icinga_checkable_test_SOURCES}
  LIBRARIES ${base_DEPS}
  TESTS icinga_checkable_flapping/host_not_flapping
        icinga_checkable_flapping/host_flapping
        icinga_checkable_flapping/host_flapping_recover
        icinga_checkable_flapping/host_flapping_docs_example
)
