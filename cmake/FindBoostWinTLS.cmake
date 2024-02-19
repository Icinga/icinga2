FIND_PATH(BoostWinTLS_HEADERS wintls.hpp HINTS "${PROJECT_SOURCE_DIR}/third-party/boost-wintls/include/boost")

if(BoostWinTLS_HEADERS)
  set(BoostWinTLS_INCLUDE "${BoostWinTLS_HEADERS}/..")
  message(STATUS "Found Boost.WinTLS: ${BoostWinTLS_INCLUDE}")
else()
  message(FATAL_ERROR "Unable to include wintls.hpp")
endif()
