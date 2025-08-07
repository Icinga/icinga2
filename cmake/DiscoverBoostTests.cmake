# Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+

# Needed because CMAKE_CURRENT_FUNCTION_LIST_DIR is 3.15+
set(DISCOVER_BOOST_TESTS_SCRIPT_DIR ${CMAKE_CURRENT_LIST_DIR})

function(target_discover_boost_tests target)
  set(testfile "${CMAKE_CURRENT_BINARY_DIR}/${target}_tests.cmake")
  set(args -- --generate_ctest_config "${testfile}")
  string(REPLACE ";" "$<SEMICOLON>" test "${args}")

  add_custom_command(TARGET "${target}" POST_BUILD
    COMMAND ${CMAKE_COMMAND} -DCMD=$<TARGET_FILE:${target}> -DARGS="${test}" -P "${DISCOVER_BOOST_TESTS_SCRIPT_DIR}/ExecuteCommandQuietly.cmake"
  )

  set_property(DIRECTORY
    APPEND PROPERTY TEST_INCLUDE_FILES "${testfile}"
  )
endfunction()
