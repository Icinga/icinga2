FIND_PATH (UTF8CPP_INCLUDE utf8.h HINTS "${PROJECT_SOURCE_DIR}/third-party/utf8cpp/source")

if (UTF8CPP_INCLUDE)
  message(STATUS "Found UTF8CPP: ${UTF8CPP_INCLUDE}" )
else ()
  message(FATAL_ERROR "Unable to include utf8.h")
endif ()
