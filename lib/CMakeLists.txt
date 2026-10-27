# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

add_library(lib_includes INTERFACE)

target_include_directories(lib_includes INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_BINARY_DIR}
)

add_subdirectory(base)
add_subdirectory(cli)
add_subdirectory(config)
add_subdirectory(remote)
add_subdirectory(icinga)
add_subdirectory(methods)

if(ICINGA2_WITH_CHECKER)
  add_subdirectory(checker)
endif()

if(ICINGA2_WITH_COMPAT)
  add_subdirectory(compat)
endif()

if(ICINGA2_WITH_MYSQL OR ICINGA2_WITH_PGSQL)
  add_subdirectory(db_ido)
endif()

if(ICINGA2_WITH_MYSQL)
  find_package(MySQL)

  if(MYSQL_FOUND)
    add_subdirectory(db_ido_mysql)
    add_subdirectory(mysql_shim)
  else()
    message(FATAL_ERROR "You have selected MySQL support, but MySQL could not be found. You can disable the MySQL IDO module using -DICINGA2_WITH_MYSQL=OFF.")
  endif()
endif()

if(ICINGA2_WITH_PGSQL)
  find_package(PostgreSQL)

  if(PostgreSQL_FOUND)
    add_subdirectory(db_ido_pgsql)
    add_subdirectory(pgsql_shim)
  else()
    message(FATAL_ERROR "You have selected PostgreSQL support, but PostgreSQL could not be found. You can disable the PostgreSQL IDO module using -DICINGA2_WITH_PGSQL=OFF.")
  endif()
endif()

if(ICINGA2_WITH_LIVESTATUS)
  add_subdirectory(livestatus)
endif()

if(ICINGA2_WITH_NOTIFICATION)
  add_subdirectory(notification)
endif()

option(ICINGA2_WITH_OPENTELEMETRY "Build the OpenTelemetry integration module" ${ICINGA2_MASTER})

if(ICINGA2_WITH_OPENTELEMETRY)
  # Newer Protobuf versions provide a CMake config package that we should prefer, since it implicitly
  # links against all its dependencies (like absl, etc.) that would otherwise need to be linked manually.
  # Thus, first try to find Protobuf in config mode and only fall back to module mode if that fails.
  find_package(Protobuf CONFIG)
  if(NOT Protobuf_FOUND)
    # FindProtobuf.cmake in CMake versions < 3.31.0 is just broken and mixes up the Protobuf output directories
    # and it doesn't even support to pass any PLUGIN_OPTIONS like "lite" to the protobuf_generate() function in
    # order to generate code for the lite runtime without having to modify the proto files directly.
    if(CMAKE_VERSION VERSION_LESS 3.31.0)
      list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/third-party/cmake/protobuf")
    endif()
    find_package(Protobuf REQUIRED)
  endif()

  if(TARGET absl::config)
    get_target_property(absl_cxx_features absl::config INTERFACE_COMPILE_FEATURES)

    set(absl_cxx_standard 0)
    if(absl_cxx_features)
      foreach(absl_cxx_feature ${absl_cxx_features})
        if(absl_cxx_feature MATCHES "^cxx_std_([0-9]+)$")
          if(CMAKE_MATCH_1 GREATER absl_cxx_standard)
            set(absl_cxx_standard ${CMAKE_MATCH_1})
          endif()
        endif()
      endforeach()
    endif()

    if(absl_cxx_standard GREATER CMAKE_CXX_STANDARD)
      message(FATAL_ERROR
        "Abseil (a transitive dependency of Protobuf, required for ICINGA2_WITH_OPENTELEMETRY) "
        "requires C++${_icinga2_absl_cxx_standard}, but this project is configured for "
        "C++${CMAKE_CXX_STANDARD} (CMAKE_CXX_STANDARD). Mixing standards across translation units "
        "in the same binary silently breaks the ABI of header-only template libraries like "
        "Boost.Asio and causes memory corruption at runtime rather than a build error. Please "
        "reconfigure with -DCMAKE_CXX_STANDARD=${_icinga2_absl_cxx_standard}."
      )
    endif()

    unset(absl_cxx_standard)
    unset(absl_cxx_features)
  endif()

  add_subdirectory(otel)
endif()

if(ICINGA2_WITH_PERFDATA)
  add_subdirectory(perfdata)
endif()

if(ICINGA2_WITH_ICINGADB)
  add_subdirectory(icingadb)
endif()

set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}" PARENT_SCOPE)
