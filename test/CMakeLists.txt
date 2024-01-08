Include(FetchContent)

find_package(Boost COMPONENTS filesystem)
find_package(OpenSSL COMPONENTS SSL Crypto)
find_package(Threads)

if(NOT OPENSSL_FOUND)
  message(SEND_ERROR "OpenSSL not found. Cannot build tests.")
  return()
endif()

if(NOT Threads_FOUND)
  message(SEND_ERROR "Threads library not found. Cannot build tests.")
  return()
endif()

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v2.13.6
)

FetchContent_MakeAvailable(Catch2)
list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/contrib)

set(test_sources
  main.cpp
  echo_test.cpp
  tls_record.cpp
  error_test.cpp
  handshake_test.cpp
  certificate_test.cpp
  sspi_buffer_sequence_test.cpp
  stream_test.cpp
  decrypted_data_buffer_test.cpp
)

add_executable(unittest
  ${test_sources}
)

if(ENABLE_ADDRESS_SANITIZER)
  # Workaround for: https://github.com/catchorg/Catch2/issues/898
  target_compile_definitions(unittest PRIVATE CATCH_CONFIG_NO_WINDOWS_SEH)
endif()

if(MSVC)
  target_compile_options(unittest PRIVATE "-bigobj")
endif()

target_compile_definitions(unittest PRIVATE
  TEST_CERTIFICATES_PATH="${CMAKE_CURRENT_LIST_DIR}/test_certificates/gen/"
)

target_link_libraries(unittest PRIVATE
  OpenSSL::SSL
  OpenSSL::Crypto
  Threads::Threads
  Catch2::Catch2
  Boost::filesystem
  boost-wintls
)

if(MSVC AND ${Boost_VERSION} VERSION_LESS "1.79" AND ${OPENSSL_VERSION} VERSION_GREATER_EQUAL "3.0")
  # A bit of a hack. Older Boost.Asio versions use OpenSSL functions
  # deprecated in version 3.0 so disable warnings on use of deprecated
  # functions on MSVC where OpenSSL headers are not included as system
  # headers.
  target_compile_options(unittest PRIVATE /wd4996)
endif()

include(CTest)
include(Catch)
catch_discover_tests(unittest TEST_SPEC "*")
