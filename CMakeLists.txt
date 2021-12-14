# Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+

cmake_minimum_required(VERSION 2.8.8)
set(BOOST_MIN_VERSION "1.66.0")

project(icinga2)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/third-party/cmake")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING
    "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
    FORCE)
endif()

option(ICINGA2_WITH_MYSQL "Build the MySQL IDO module" ON)
option(ICINGA2_WITH_PGSQL "Build the PostgreSQL IDO module" ON)
option(ICINGA2_WITH_CHECKER "Build the checker module" ON)
option(ICINGA2_WITH_COMPAT "Build the compat module" ON)
option(ICINGA2_WITH_LIVESTATUS "Build the Livestatus module" ON)
option(ICINGA2_WITH_NOTIFICATION "Build the notification module" ON)
option(ICINGA2_WITH_PERFDATA "Build the perfdata module" ON)
option(ICINGA2_WITH_TESTS "Run unit tests" ON)

# IcingaDB only is supported on modern Linux/Unix master systems
if(NOT WIN32)
  option(ICINGA2_WITH_ICINGADB "Build the IcingaDB module" ON)
else()
  option(ICINGA2_WITH_ICINGADB "Build the IcingaDB module" OFF)
endif()

option (USE_SYSTEMD
 "Configure icinga as native systemd service instead of a SysV initscript" OFF)

set(HAVE_SYSTEMD ${USE_SYSTEMD})

include(GNUInstallDirs)
include(InstallConfig)
include(SetFullDir)

set(ICINGA2_USER "icinga" CACHE STRING "Icinga 2 user")
set(ICINGA2_GROUP "icinga" CACHE STRING "Icinga 2 group")
set(ICINGA2_COMMAND_GROUP "icingacmd" CACHE STRING "Icinga 2 command group")
set(ICINGA2_PLUGINDIR "/usr/lib/nagios/plugins" CACHE STRING "Path for the check plugins")
set(ICINGA2_GIT_VERSION_INFO ON CACHE BOOL "Whether to use git describe")
set(ICINGA2_UNITY_BUILD ON CACHE BOOL "Whether to perform a unity build")
set(ICINGA2_LTO_BUILD OFF CACHE BOOL "Whether to use LTO")

set(ICINGA2_CONFIGDIR "${CMAKE_INSTALL_SYSCONFDIR}/icinga2" CACHE FILEPATH "Main config directory, e.g. /etc/icinga2")
set(ICINGA2_CACHEDIR "${CMAKE_INSTALL_LOCALSTATEDIR}/cache/icinga2" CACHE FILEPATH "Directory for cache files, e.g. /var/cache/icinga2")
set(ICINGA2_DATADIR "${CMAKE_INSTALL_LOCALSTATEDIR}/lib/icinga2" CACHE FILEPATH "Data directory for the daemon, e.g. /var/lib/icinga2")
set(ICINGA2_LOGDIR "${CMAKE_INSTALL_LOCALSTATEDIR}/log/icinga2" CACHE FILEPATH "Logging directory, e.g. /var/log/icinga2")
set(ICINGA2_SPOOLDIR "${CMAKE_INSTALL_LOCALSTATEDIR}/spool/icinga2" CACHE FILEPATH "Spooling directory, e.g. /var/spool/icinga2")
set(ICINGA2_RUNDIR "${CMAKE_INSTALL_LOCALSTATEDIR}/run" CACHE STRING "/run directory (deprecated, please use ICINGA2_INITRUNDIR)")
set(ICINGA2_INITRUNDIR "${ICINGA2_RUNDIR}/icinga2" CACHE FILEPATH "Runtime data for the init system, e.g. /run/icinga2")

set(ICINGA2_PKGDATADIR "${CMAKE_INSTALL_DATADIR}/icinga2" CACHE FILEPATH "Installed data, e.g. /usr/share/icinga2")
set(ICINGA2_INCLUDEDIR "${ICINGA2_PKGDATADIR}/include" CACHE FILEPATH "Include directory for the ITL, e.g. /usr/share/icinga2/include")

# ensure absolute paths
set_full_dir(ICINGA2_FULL_CONFIGDIR "${ICINGA2_CONFIGDIR}")
set_full_dir(ICINGA2_FULL_CACHEDIR "${ICINGA2_CACHEDIR}")
set_full_dir(ICINGA2_FULL_DATADIR "${ICINGA2_DATADIR}")
set_full_dir(ICINGA2_FULL_LOGDIR "${ICINGA2_LOGDIR}")
set_full_dir(ICINGA2_FULL_SPOOLDIR "${ICINGA2_SPOOLDIR}")
set_full_dir(ICINGA2_FULL_RUNDIR "${ICINGA2_RUNDIR}")
set_full_dir(ICINGA2_FULL_INITRUNDIR "${ICINGA2_INITRUNDIR}")
set_full_dir(ICINGA2_FULL_PKGDATADIR "${ICINGA2_PKGDATADIR}")
set_full_dir(ICINGA2_FULL_INCLUDEDIR "${ICINGA2_INCLUDEDIR}")

set(LOGROTATE_DIR "${CMAKE_INSTALL_SYSCONFDIR}/logrotate.d" CACHE STRING "Location of logrotate configs, e.g. /etc/logrotate.d")
set(BASHCOMPLETION_DIR "${CMAKE_INSTALL_SYSCONFDIR}/bash_completion.d" CACHE STRING "Location of bash_completion files, e.g. /etc/bash_completion.d")

if(NOT WIN32)
  set(ICINGA2_SYSCONFIGFILE "${CMAKE_INSTALL_SYSCONFDIR}/sysconfig/icinga2" CACHE PATH "where to store configuation for the init system, defaults to /etc/sysconfig/icinga2")
endif()

site_name(ICINGA2_BUILD_HOST_NAME)
set(ICINGA2_BUILD_COMPILER_NAME "${CMAKE_CXX_COMPILER_ID}")

if(NOT CMAKE_CXX_COMPILER_VERSION)
  execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion
    OUTPUT_VARIABLE CMAKE_CXX_COMPILER_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

set(ICINGA2_BUILD_COMPILER_VERSION "${CMAKE_CXX_COMPILER_VERSION}")

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/COPYING" ICINGA2_LICENSE_GPL)
set(ICINGA2_LICENSE "${ICINGA2_LICENSE_GPL}\n\n---\n\n${ICINGA2_LICENSE_ADDITIONS}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/LICENSE.txt" ${ICINGA2_LICENSE})

file(STRINGS ICINGA2_VERSION SPEC_VERSION REGEX "^Version:")
string(LENGTH "${SPEC_VERSION}" SPEC_VERSION_LENGTH)
math(EXPR SPEC_VERSION_LENGTH "${SPEC_VERSION_LENGTH} - 9")
string(SUBSTRING ${SPEC_VERSION} 9 ${SPEC_VERSION_LENGTH} SPEC_VERSION)

configure_file(icinga-spec-version.h.cmake icinga-spec-version.h)

include(GetGitRevisionDescription)
git_describe(GIT_VERSION --tags)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/icinga-version.h.force)
  configure_file(icinga-version.h.force ${CMAKE_CURRENT_BINARY_DIR}/icinga-version.h COPYONLY)
else()
  if(NOT ICINGA2_GIT_VERSION_INFO OR GIT_VERSION MATCHES "-NOTFOUND$")
    file(STRINGS ICINGA2_VERSION SPEC_REVISION REGEX "^Revision: ")
    string(LENGTH "${SPEC_REVISION}" SPEC_REVISION_LENGTH)
    math(EXPR SPEC_REVISION_LENGTH "${SPEC_REVISION_LENGTH} - 10")
    string(SUBSTRING ${SPEC_REVISION} 10 ${SPEC_REVISION_LENGTH} SPEC_REVISION)

    set(GIT_VERSION "r${SPEC_VERSION}-${SPEC_REVISION}")
    set(ICINGA2_VERSION "${SPEC_VERSION}")
  else()
    # use GIT version as ICINGA2_VERSION
    string(REGEX REPLACE "^[rv]" "" ICINGA2_VERSION "${GIT_VERSION}")
  endif()
  configure_file(icinga-version.h.cmake icinga-version.h)
endif()

# NuGet on Windows requires a semantic versioning, example: 2.10.4.123 (only 4 element, only numeric)
string(REGEX REPLACE "-([0-9]+).*$" ".\\1" ICINGA2_VERSION_SAFE "${ICINGA2_VERSION}")
string(REGEX REPLACE "-[^\\.]*(.*)$" "\\1" ICINGA2_VERSION_SAFE "${ICINGA2_VERSION_SAFE}")
string(REGEX REPLACE "^([0-9]+\\.[0-9]+\\.[0-9]+)[\\.]?[0-9]*" "\\1" CHOCO_VERSION_SHORT "${ICINGA2_VERSION_SAFE}")

message(STATUS "ICINGA2_VERSION_SAFE=${ICINGA2_VERSION_SAFE} CHOCO_VERSION_SHORT=${CHOCO_VERSION_SHORT}")

if(WIN32)
  set(Boost_USE_STATIC_LIBS ON)
  # Disabled for linking issues for newer Boost versions, they link against Windows SDKs
  #add_definitions(-DBOOST_ALL_NO_LIB)

  # Disable optimization for Boost::context
  # https://www.boost.org/doc/libs/1_69_0/libs/context/doc/html/context/overview.html
  # https://docs.microsoft.com/en-us/cpp/build/reference/gl-whole-program-optimization?view=vs-2017
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /bigobj /GL- /EHs")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj /GL- /EHs")

  # detect if 32-bit target
  if(CMAKE_VS_PLATFORM_NAME STREQUAL "Win32")
    # SAFESEH is not supported in Boost on Windows x86
    # maybe it is when Boost is compiled with it...
    # https://lists.boost.org/Archives/boost/2013/10/206720.php
    # https://docs.microsoft.com/en-us/cpp/build/reference/safeseh-image-has-safe-exception-handlers?view=vs-2017
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
  endif()
endif()

if(NOT DEFINED LOGROTATE_HAS_SU)
  set(LOGROTATE_HAS_SU OFF)
  find_program(LOGROTATE_BINARY logrotate)
  execute_process(COMMAND ${LOGROTATE_BINARY} ERROR_VARIABLE LOGROTATE_OUTPUT)
  if(LOGROTATE_OUTPUT)
    string(REGEX REPLACE "^logrotate ([0-9.]*).*" "\\1" LOGROTATE_VERSION
      ${LOGROTATE_OUTPUT})
    message(STATUS "Found logrotate (found version \"${LOGROTATE_VERSION}\")")
    if("${LOGROTATE_VERSION}" VERSION_GREATER "3.7.9")
      set(LOGROTATE_HAS_SU ON)
    endif()
  endif()
endif()
if(LOGROTATE_HAS_SU)
  set(LOGROTATE_USE_SU "\n\tsu ${ICINGA2_USER} ${ICINGA2_GROUP}")
else()
  set(LOGROTATE_CREATE "\n\tcreate 644 ${ICINGA2_USER} ${ICINGA2_GROUP}")
endif()

find_package(Boost ${BOOST_MIN_VERSION} COMPONENTS coroutine context date_time filesystem thread system program_options regex REQUIRED)

# Boost.Coroutine2 (the successor of Boost.Coroutine)
# (1) doesn't even exist in old Boost versions and
# (2) isn't supported by ASIO, yet.
add_definitions(-DBOOST_COROUTINES_NO_DEPRECATION_WARNING)

add_definitions(-DBOOST_FILESYSTEM_NO_DEPRECATED)

# Required for Boost v1.74+
add_definitions(-DBOOST_ASIO_USE_TS_EXECUTOR_AS_DEFAULT)

link_directories(${Boost_LIBRARY_DIRS})
include_directories(${Boost_INCLUDE_DIRS})

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

set(base_DEPS ${CMAKE_DL_LIBS} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES})
set(base_OBJS $<TARGET_OBJECTS:mmatch> $<TARGET_OBJECTS:socketpair> $<TARGET_OBJECTS:base>)

# JSON
find_package(JSON)
include_directories(${JSON_INCLUDE})

# UTF8CPP
find_package(UTF8CPP)
include_directories(${UTF8CPP_INCLUDE})

find_package(Editline)
set(HAVE_EDITLINE "${EDITLINE_FOUND}")

find_package(Termcap)
set(HAVE_TERMCAP "${TERMCAP_FOUND}")

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/lib
  ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/lib
)

if(UNIX OR CYGWIN)
  list(APPEND base_OBJS $<TARGET_OBJECTS:execvpe>)
endif()

if(HAVE_SYSTEMD)
  list(APPEND base_DEPS systemd)
endif()

if(EDITLINE_FOUND)
  list(APPEND base_DEPS ${EDITLINE_LIBRARIES})
  include_directories(${EDITLINE_INCLUDE_DIR})
endif()

if(TERMCAP_FOUND)
  list(APPEND base_DEPS ${TERMCAP_LIBRARIES})
  include_directories(${TERMCAP_INCLUDE_DIR})
endif()

if(WIN32)
  list(APPEND base_DEPS ws2_32 dbghelp shlwapi msi)
endif()

set(CMAKE_MACOSX_RPATH 1)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH};${CMAKE_INSTALL_FULL_LIBDIR}/icinga2")

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Qunused-arguments -fcolor-diagnostics")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Qunused-arguments -fcolor-diagnostics")

  # Clang on Fedora requires -pthread, Apple Clang does not
  # AppleClang is available since CMake 3.0.0
  if (NOT CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
  endif()
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "SunPro")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mt")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mt -library=stlport4")
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  if(CMAKE_SYSTEM_NAME MATCHES AIX)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -lpthread")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -lpthread")
  elseif(CMAKE_SYSTEM_NAME MATCHES "kOpenBSD.*|OpenBSD.*")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -pthread")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -pthread")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lpthread")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -lpthread")
    set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} -lpthread")
  else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -pthread")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -pthread")
  endif()
endif()

include(CheckCXXCompilerFlag)

function(check_cxx_linker_flag flag var)
  set(CMAKE_REQUIRED_FLAGS ${flag})
  set(result 0)
  check_cxx_compiler_flag(${flag} result)
  set(${var} ${result} PARENT_SCOPE)
endfunction()

check_cxx_linker_flag("-Wl,--gc-sections" LD_GC_SECTIONS)

if(LD_GC_SECTIONS)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections")
endif()

check_cxx_linker_flag("-Wl,--no-export-dynamic" LD_NO_EXPORT_DYNAMIC)

if(LD_NO_EXPORT_DYNAMIC)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-export-dynamic")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-export-dynamic")
endif()

check_cxx_linker_flag("-Bsymbolic-functions" LD_SYMBOLIC_FUNCTIONS)

if(LD_SYMBOLIC_FUNCTIONS)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Bsymbolic-functions")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Bsymbolic-functions")
endif()

check_cxx_linker_flag("-Wl,--dynamic-list-cpp-typeinfo" LD_DYNAMIC_LIST_CPP_TYPEINFO)

if(LD_DYNAMIC_LIST_CPP_TYPEINFO)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--dynamic-list-cpp-typeinfo")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--dynamic-list-cpp-typeinfo")
endif()

check_cxx_linker_flag("-Wl,--dynamic-list-data" LD_DYNAMIC_LIST_DATA)

if(LD_DYNAMIC_LIST_DATA)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--dynamic-list-data")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--dynamic-list-data")
endif()

check_cxx_compiler_flag("-Winvalid-pch" CXX_INVALID_PCH)

if(CXX_INVALID_PCH)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Winvalid-pch")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Winvalid-pch")
endif()

if(ICINGA2_LTO_BUILD)
  check_cxx_compiler_flag("-flto" CXX_FLAG_LTO)

  if(NOT CXX_FLAG_LTO)
    message(WARNING "Compiler does not support LTO, falling back to non-LTO build")
  else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -flto")

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9.0) AND NOT OPENBSD)
      set(CMAKE_AR "gcc-ar")
      set(CMAKE_RANLIB "gcc-ranlib")
    endif()
  endif()
endif()

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE -D_SCL_SECURE_NO_WARNINGS)
endif()

set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/Bin/${CMAKE_BUILD_TYPE} CACHE PATH "Library output path")
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/Bin/${CMAKE_BUILD_TYPE} CACHE PATH "Executable output path")

include(CheckSymbolExists)
include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckIncludeFileCXX)

check_symbol_exists(__COUNTER__ "" HAVE_COUNTER_MACRO)

if(NOT HAVE_COUNTER_MACRO)
message(FATAL_ERROR "Your C/C++ compiler does not support the __COUNTER__ macro.")
endif()

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DI2_DEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DI2_DEBUG")

check_function_exists(vfork HAVE_VFORK)
check_function_exists(backtrace_symbols HAVE_BACKTRACE_SYMBOLS)
check_function_exists(pipe2 HAVE_PIPE2)
check_function_exists(nice HAVE_NICE)
check_library_exists(dl dladdr "dlfcn.h" HAVE_DLADDR)
check_library_exists(execinfo backtrace_symbols "" HAVE_LIBEXECINFO)
check_include_file_cxx(cxxabi.h HAVE_CXXABI_H)

if(HAVE_LIBEXECINFO)
  set(HAVE_BACKTRACE_SYMBOLS TRUE)
  list(APPEND base_DEPS execinfo)
endif()

if(NOT WIN32)
  # boost::stacktrace uses _Unwind_Backtrace which is only exposed if _GNU_SOURCE is defined on most systems
  add_definitions(-D_GNU_SOURCE)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
  set(ICINGA2_STACKTRACE_USE_BACKTRACE_SYMBOLS TRUE)
endif()

if(ICINGA2_STACKTRACE_USE_BACKTRACE_SYMBOLS AND NOT HAVE_BACKTRACE_SYMBOLS)
  message(FATAL_ERROR "ICINGA2_STACKTRACE_USE_BACKTRACE_SYMBOLS is set but backtrace_symbols() was not found")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  exec_program(${CMAKE_CXX_COMPILER}
    ARGS -dumpversion
    OUTPUT_VARIABLE _ICINGA2_COMPILER_VERSION
  )

  if("${CMAKE_CXX_COMPILER_VERSION}" VERSION_LESS "6.3.0")
    message(FATAL_ERROR "Your version of GCC (${CMAKE_CXX_COMPILER_VERSION}) is too old for building Icinga 2 (GCC >= 6.3.0 is required).")
  endif()
endif()

if(MSVC)
  if("${CMAKE_CXX_COMPILER_VERSION}" VERSION_LESS "19.20")
    message(FATAL_ERROR "Your version of MSVC (${CMAKE_CXX_COMPILER_VERSION}) is too old for building Icinga 2 (MSVC >= 19.20 from Visual Studio 2019 is required).")
  endif()
endif()

if(NOT MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

  check_cxx_source_compiles("class Base { public: virtual void test(void) { } }; class Derived : public Base { virtual void test(void) override { } }; int main(){}" CXX_FEATURE_OVERRIDE)

  if(NOT CXX_FEATURE_OVERRIDE)
    add_definitions("-Doverride=")
  endif()
endif()

# Architecture specifics
# - Log the target architecture
# - ARM needs to link against atomic
if(NOT MSVC)
  # inspired by https://github.com/civetweb/civetweb/blob/master/cmake/DetermineTargetArchitecture.cmake
  execute_process(
    COMMAND ${CMAKE_C_COMPILER} -dumpmachine
    RESULT_VARIABLE RESULT
    OUTPUT_VARIABLE ARCH
    ERROR_QUIET
  )

  if (RESULT)
    message(STATUS "Failed to detect target architecture with compiler ${CMAKE_C_COMPILER}: ${RESULT}")
  endif()

  string(REGEX MATCH "([^-]+).*" ARCH_MATCH "${ARCH}")
  if (NOT CMAKE_MATCH_1 OR NOT ARCH_MATCH)
    message(STATUS "Failed to match the target architecture: ${ARCH}")
  endif()

  set(ARCH ${CMAKE_MATCH_1})

  message(STATUS "Target architecture - ${ARCH}")

  # ARM settings
  if("${ARCH}" STREQUAL "arm")
    check_cxx_source_compiles( "include <atomic>; int main(){ std::atomic<uint_fast64_t> x; x.fetch_add(1); x.sub_add(1);  }" CXX_ATOMIC)
    link_libraries(atomic)
  endif()

else()
  if("${MSVC_C_ARCHITECTURE_ID}" STREQUAL "X86")
    set(ARCH "i686")
  elseif("${MSVC_C_ARCHITECTURE_ID}" STREQUAL "x64")
    set(ARCH "x86_64")
  elseif("${MSVC_C_ARCHITECTURE_ID}" STREQUAL "ARM")
    set(ARCH "arm")
  else()
    message(FATAL_ERROR "Failed to determine the MSVC target architecture: ${MSVC_C_ARCHITECTURE_ID}")
  endif()

  message(STATUS "Target architecture - ${ARCH}")
endif()

configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h ESCAPE_QUOTES)

install(
  FILES README.md COPYING AUTHORS CHANGELOG.md NEWS
  DESTINATION ${CMAKE_INSTALL_DOCDIR}
)

include(CTest)
enable_testing()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory(third-party)
add_subdirectory(tools)
add_subdirectory(lib)
add_subdirectory(icinga-app)
add_subdirectory(etc)
add_subdirectory(itl)
add_subdirectory(doc)
add_subdirectory(agent)
add_subdirectory(plugins)
add_subdirectory(choco)

if(MSVC)
  add_subdirectory(icinga-installer)
endif()

if(ICINGA2_WITH_TESTS)
  add_subdirectory(test)
endif()

set(CPACK_PACKAGE_NAME "Icinga 2")
set(CPACK_PACKAGE_VENDOR "Icinga GmbH")
set(CPACK_PACKAGE_VERSION ${ICINGA2_VERSION_SAFE})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "ICINGA2")
set(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/icinga-app\\\\icinga.ico")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_BINARY_DIR}/LICENSE.txt")

set(CPACK_PACKAGE_EXECUTABLES "Icinga2SetupAgent;Icinga 2 Agent Wizard")
set(CPACK_WIX_PRODUCT_ICON "${CMAKE_CURRENT_SOURCE_DIR}/icinga-app\\\\icinga.ico")
set(CPACK_WIX_UPGRADE_GUID "52F2BEAA-4DF0-4C3E-ABDC-C0F61DE4DF8A")
set(CPACK_WIX_UI_BANNER "${CMAKE_CURRENT_SOURCE_DIR}/icinga-installer/bannrbmp.bmp")
set(CPACK_WIX_UI_DIALOG "${CMAKE_CURRENT_SOURCE_DIR}/icinga-installer/dlgbmp.bmp")
set(CPACK_WIX_PATCH_FILE "${CMAKE_CURRENT_BINARY_DIR}/icinga-installer/icinga2.wixpatch.Debug")
set(CPACK_WIX_PATCH_FILE "${CMAKE_CURRENT_BINARY_DIR}/icinga-installer/icinga2.wixpatch")
set(CPACK_WIX_EXTENSIONS "WixUtilExtension" "WixNetFxExtension")

set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION "sbin")
set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
include(InstallRequiredSystemLibraries)

if(WIN32)
  if(CMAKE_VS_PLATFORM_NAME STREQUAL "x64")
    set(NSCP_URL "https://github.com/mickem/nscp/releases/download/0.5.2.39/NSCP-0.5.2.39-x64.msi")
    set(NSCP_SHA256 "dfe93c293f30586b02510d8b7884e4e177b93a5fead8b5dc6de8103532e6e159")
  else()
    set(NSCP_URL "https://github.com/mickem/nscp/releases/download/0.5.2.39/NSCP-0.5.2.39-Win32.msi")
    set(NSCP_SHA256 "ca6a67fb01c1468f2b510fd2f9eb0750887db3fb49a0302732c1421c85c6627c")
  endif()

  set(NSCP_SHA256SUM "")
  if (EXISTS ${CMAKE_CURRENT_BINARY_DIR}/NSCP.msi)
    file(SHA256 ${CMAKE_CURRENT_BINARY_DIR}/NSCP.msi NSCP_SHA256SUM)
  endif()

  if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/NSCP.msi OR NOT ${NSCP_SHA256SUM} STREQUAL ${NSCP_SHA256})
    file(DOWNLOAD ${NSCP_URL} ${CMAKE_CURRENT_BINARY_DIR}/NSCP.msi SHOW_PROGRESS)
  endif()

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/NSCP.msi DESTINATION ${CMAKE_INSTALL_SBINDIR})

  if (OPENSSL_VERSION_MINOR GREATER_EQUAL 1)
    if (CMAKE_VS_PLATFORM_NAME STREQUAL "x64")
      list (APPEND ICINGA2_OPENSSL_DLLS ${OPENSSL_INCLUDE_DIR}/../bin/libcrypto-1_1-x64.dll ${OPENSSL_INCLUDE_DIR}/../bin/libssl-1_1-x64.dll)
    else()
      list (APPEND ICINGA2_OPENSSL_DLLS ${OPENSSL_INCLUDE_DIR}/../bin/libcrypto-1_1.dll ${OPENSSL_INCLUDE_DIR}/../bin/libssl-1_1.dll)
    endif()
  else()
      list (APPEND ICINGA2_OPENSSL_DLLS ${OPENSSL_INCLUDE_DIR}/../bin/libeay32.dll ${OPENSSL_INCLUDE_DIR}/../bin/ssleay32.dll)
  endif()

  install(
    PROGRAMS ${ICINGA2_OPENSSL_DLLS}
    DESTINATION ${CMAKE_INSTALL_SBINDIR}
  )
endif()

include(CPack)
