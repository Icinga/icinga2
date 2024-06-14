add_executable(https_client https_client.cpp)
add_executable(async_https_client async_https_client.cpp)
add_executable(echo_client echo_client.cpp)
add_executable(echo_server echo_server.cpp)

target_link_libraries(https_client PRIVATE
  boost-wintls
)

target_link_libraries(async_https_client PRIVATE
  boost-wintls
)

target_link_libraries(echo_server PRIVATE
  boost-wintls
)

target_link_libraries(echo_client PRIVATE
  boost-wintls
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # Temporary workaround issue https://github.com/boostorg/beast/issues/1582
  target_compile_options(https_client PRIVATE "-wd4702")
  target_compile_options(async_https_client PRIVATE "-wd4702")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  target_compile_options(echo_client PRIVATE -Wno-unused-private-field)
  target_compile_options(echo_server PRIVATE -Wno-unused-private-field)
  target_compile_options(https_client PRIVATE -Wno-unused-private-field)
  target_compile_options(async_https_client PRIVATE -Wno-unused-private-field)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # Work around string table overflow by enabling optimizations
  target_compile_options(async_https_client PRIVATE -Os)
endif()
