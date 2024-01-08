file(GLOB_RECURSE ALL_SOURCE_FILES *.cpp *.hpp)
foreach (SOURCE_FILE ${ALL_SOURCE_FILES})
  string(FIND ${SOURCE_FILE} ${PROJECT_BINARY_DIR} PROJECT_BINARY_DIR_FOUND)
  if (NOT ${PROJECT_BINARY_DIR_FOUND} EQUAL -1)
    list(REMOVE_ITEM ALL_SOURCE_FILES ${SOURCE_FILE})
  endif ()
endforeach ()

add_custom_target(
  clangformat
  COMMAND clang-format
  -i
  ${ALL_SOURCE_FILES}
)
