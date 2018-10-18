FIND_PATH (JSON_INCLUDE json.hpp HINTS "${PROJECT_SOURCE_DIR}/third-party/nlohmann_json/single_include/nlohmann")

if (JSON_INCLUDE)
  set(JSON_BuildTests OFF CACHE INTERNAL "")

  message(STATUS "Found JSON: ${JSON_INCLUDE}" )
else ()
  message(FATAL_ERROR "Unable to include json.hpp")
endif ()
