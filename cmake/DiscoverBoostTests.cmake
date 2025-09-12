# Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+

# - Discover tests defined in the Boost.Test executable target
#
# Boost.Test executables should be defined like any other CMake executable target.
# Then, this function can be used on the executable target to discover all the unit
# tests in that executable.
# This relies on the additional commandline argument added in 'test/test-ctest.hpp'
function(target_discover_boost_tests target)
  set(testfile "${CMAKE_CURRENT_BINARY_DIR}/${target}_tests.cmake")
  set(args -- --generate_ctest_config "${testfile}")
  string(REPLACE ";" "$<SEMICOLON>" test "${args}")

  add_custom_command(TARGET "${target}" POST_BUILD
    COMMAND ${CMAKE_COMMAND} -DCMD=$<TARGET_FILE:${target}> -DARGS="${test}" -P "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/ExecuteCommandQuietly.cmake"
  )

  set_property(DIRECTORY
    APPEND PROPERTY TEST_INCLUDE_FILES "${testfile}"
  )
endfunction()
