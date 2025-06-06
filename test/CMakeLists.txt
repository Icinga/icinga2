# Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+

include(BoostTestTargets)

set(types_test_SOURCES
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
add_boost_test(types
  SOURCES test-runner.cpp ${types_test_SOURCES}
  LIBRARIES ${base_DEPS}
  TESTS
    types/gettype
    types/assign
    types/byname
    types/instantiate
    types/sort_by_load_after
)

set(base_test_SOURCES
  icingaapplication-fixture.cpp
  utils.cpp
  base-array.cpp
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
  remote-configpackageutility.cpp
  remote-url.cpp
  ${base_OBJS}
  $<TARGET_OBJECTS:config>
  $<TARGET_OBJECTS:remote>
  $<TARGET_OBJECTS:icinga>
  $<TARGET_OBJECTS:methods>
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
    base_io_engine/timeout_run
    base_io_engine/timeout_cancelled
    base_io_engine/timeout_scope
    base_io_engine/timeout_due_cancelled
    base_io_engine/timeout_due_scope
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
    base_string/vector_move
    base_string/move_string_out_of_Value_type
    base_timer/construct
    base_timer/interval
    base_timer/invoke
    base_timer/scope
    base_tlsutility/sha1
    base_tlsutility/iscauptodate_ok
    base_tlsutility/iscauptodate_expiring
    base_tlsutility/iscertuptodate_ok
    base_tlsutility/iscertuptodate_expiring
    base_tlsutility/iscertuptodate_old
    base_tlsutility/VerifyCertificate_revalidate
    base_utility/parse_version
    base_utility/compare_version
    base_utility/comparepasswords_works
    base_utility/comparepasswords_issafe
    base_utility/validateutf8
    base_utility/EscapeCreateProcessArg
    base_utility/TruncateUsingHash
    base_utility/FormatDateTime
    base_utility/NormalizeTm
    base_value/scalar
    base_value/convert
    base_value/format
    config_apply/gettargethosts_literal
    config_apply/gettargethosts_const
    config_apply/gettargethosts_swapped
    config_apply/gettargethosts_two
    config_apply/gettargethosts_three
    config_apply/gettargethosts_mixed
    config_apply/gettargethosts_redundant
    config_apply/gettargethosts_badconst
    config_apply/gettargethosts_notliteral
    config_apply/gettargethosts_wrongop
    config_apply/gettargethosts_wrongattr
    config_apply/gettargethosts_wrongvar
    config_apply/gettargethosts_noindexer
    config_apply/gettargetservices_literal
    config_apply/gettargetservices_const
    config_apply/gettargetservices_swapped_outer
    config_apply/gettargetservices_swapped_inner
    config_apply/gettargetservices_two
    config_apply/gettargetservices_three
    config_apply/gettargetservices_mixed
    config_apply/gettargetservices_redundant
    config_apply/gettargetservices_badconst
    config_apply/gettargetservices_notliteral
    config_apply/gettargetservices_wrongop_outer
    config_apply/gettargetservices_wrongop_host
    config_apply/gettargetservices_wrongop_service
    config_apply/gettargetservices_wrongattr_host
    config_apply/gettargetservices_wrongattr_service
    config_apply/gettargetservices_wrongvar_host
    config_apply/gettargetservices_wrongvar_service
    config_apply/gettargetservices_noindexer_host
    config_apply/gettargetservices_noindexer_service
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
    icinga_checkresult/suppressed_notification
    icinga_dependencies/multi_parent
    icinga_dependencies/push_dependency_groups_to_registry
    icinga_dependencies/default_redundancy_group_registration_unregistration
    icinga_dependencies/simple_redundancy_group_registration_unregistration
    icinga_dependencies/mixed_redundancy_group_registration_unregsitration
    icinga_notification/strings
    icinga_notification/state_filter
    icinga_notification/type_filter
    icinga_notification/no_filter_problem_no_duplicate
    icinga_notification/filter_problem_no_duplicate
    icinga_notification/volatile_filter_problem_duplicate
    icinga_notification/no_recovery_filter_no_duplicate
    icinga_notification/recovery_filter_duplicate
    icinga_macros/simple
    icinga_legacytimeperiod/simple
    icinga_legacytimeperiod/is_in_range
    icinga_legacytimeperiod/out_of_range_segments
    icinga_legacytimeperiod/include_exclude_timeperiods
    icinga_legacytimeperiod/advanced
    icinga_legacytimeperiod/dst
    icinga_legacytimeperiod/dst_isinside
    icinga_legacytimeperiod/find_nth_weekday
    icinga_perfdata/empty
    icinga_perfdata/simple
    icinga_perfdata/quotes
    icinga_perfdata/multiple
    icinga_perfdata/multiline
    icinga_perfdata/normalize
    icinga_perfdata/uom
    icinga_perfdata/warncritminmax
    icinga_perfdata/ignore_warn_crit_ranges
    icinga_perfdata/invalid
    icinga_perfdata/multi
    icinga_perfdata/scientificnotation
    icinga_perfdata/parse_edgecases
    icinga_perfdata/empty_warn_crit_min_max
    methods_pluginnotificationtask/truncate_long_output
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
