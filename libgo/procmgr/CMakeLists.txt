# Icinga 2 | (c) 2019 Icinga GmbH | GPLv2+

add_custom_target(
  procmgr
  "${GO}" build -buildmode=c-archive -o "${CMAKE_CURRENT_BINARY_DIR}/libgoprocmgr4c.a" .
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
