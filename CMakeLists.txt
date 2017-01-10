# Icinga 2
# Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation
# Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

cmake_minimum_required(VERSION 2.6)
set(BOOST_MIN_VERSION "1.41.0")

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
option(ICINGA2_WITH_DEMO "Build the demo module" OFF)
option(ICINGA2_WITH_HELLO "Build the hello module" OFF)
option(ICINGA2_WITH_LIVESTATUS "Build the Livestatus module" ON)
option(ICINGA2_WITH_NOTIFICATION "Build the notification module" ON)
option(ICINGA2_WITH_PERFDATA "Build the perfdata module" ON)
option(ICINGA2_WITH_STUDIO "Build the Icinga Studio application" OFF)
option(ICINGA2_WITH_TESTS "Run unit tests" ON)

file(STRINGS icinga2.spec VERSION_LINE REGEX "^Version: ")
string(REPLACE "Version: " "" ICINGA2_VERSION ${VERSION_LINE})

include(GNUInstallDirs)
include(InstallConfig)

set(ICINGA2_USER "icinga" CACHE STRING "Icinga 2 user")
set(ICINGA2_GROUP "icinga" CACHE STRING "Icinga 2 group")
set(ICINGA2_COMMAND_GROUP "icingacmd" CACHE STRING "Icinga 2 command group")
set(ICINGA2_RUNDIR "${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/run" CACHE STRING "/run directory")
set(ICINGA2_PLUGINDIR "/usr/lib/nagios/plugins" CACHE STRING "Path for the check plugins")
set(ICINGA2_GIT_VERSION_INFO ON CACHE BOOL "Whether to use git describe")
set(ICINGA2_UNITY_BUILD ON CACHE BOOL "Whether to perform a unity build")
set(ICINGA2_LTO_BUILD OFF CACHE BOOL "Whether to use LTO")

site_name(ICINGA2_BUILD_HOST_NAME)
set(ICINGA2_BUILD_COMPILER_NAME "${CMAKE_CXX_COMPILER_ID}")

if(NOT CMAKE_CXX_COMPILER_VERSION)
  execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion
    OUTPUT_VARIABLE CMAKE_CXX_COMPILER_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

set(ICINGA2_BUILD_COMPILER_VERSION "${CMAKE_CXX_COMPILER_VERSION}")

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/COPYING" ICINGA2_LICENSE_GPL)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/COPYING.Exceptions" ICINGA2_LICENSE_ADDITIONS)
set(ICINGA2_LICENSE "${ICINGA2_LICENSE_GPL}\n\n---\n\n${ICINGA2_LICENSE_ADDITIONS}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/LICENSE.txt" ${ICINGA2_LICENSE})

file(STRINGS icinga2.spec SPEC_VERSION REGEX "^Version:")
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
    file(STRINGS icinga2.spec SPEC_REVISION REGEX "^%define revision ")
    string(LENGTH "${SPEC_REVISION}" SPEC_REVISION_LENGTH)
    math(EXPR SPEC_REVISION_LENGTH "${SPEC_REVISION_LENGTH} - 17")
    string(SUBSTRING ${SPEC_REVISION} 17 ${SPEC_REVISION_LENGTH} SPEC_REVISION)

    set(GIT_VERSION "r${SPEC_VERSION}-${SPEC_REVISION}")
  endif()
  configure_file(icinga-version.h.cmake icinga-version.h)
endif()

if(WIN32)
  set(Boost_USE_STATIC_LIBS ON)
  add_definitions(-DBOOST_ALL_NO_LIB)
  add_definitions(/bigobj)
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
endif()

find_package(Boost ${BOOST_MIN_VERSION} COMPONENTS thread system program_options regex REQUIRED)

link_directories(${Boost_LIBRARY_DIRS})
include_directories(${Boost_INCLUDE_DIRS})

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

find_package(YAJL)

if(NOT YAJL_FOUND)
  include_directories(${icinga2_BINARY_DIR}/third-party/yajl/include)
  link_directories(${icinga2_BINARY_DIR}/third-party/yajl)
  set(YAJL_LIBRARIES "yajl")
endif()

find_package(Editline)
set(HAVE_EDITLINE "${EDITLINE_FOUND}")

find_package(Termcap)
set(HAVE_TERMCAP "${TERMCAP_FOUND}")

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/lib
  ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/lib
)

set(CMAKE_MACOSX_RPATH 1)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Qunused-arguments -g")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Qunused-arguments -g")
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "SunPro")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mt")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mt -library=stlport4")
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  if(CMAKE_SYSTEM_NAME MATCHES AIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -g -lpthread")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -lpthread")
  elseif(CMAKE_SYSTEM_NAME MATCHES "kOpenBSD.*|OpenBSD.*")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -pthread")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -pthread")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lpthread")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -lpthread")
  else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -pthread")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -pthread")
  endif()

  if(NOT CMAKE_SYSTEM_NAME MATCHES AIX AND NOT CMAKE_SYSTEM_NAME MATCHES OpenBSD AND NOT CMAKE_SYSTEM_NAME MATCHES SunOS)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections -Wl,--no-export-dynamic -Bsymbolic-functions -Wl,--dynamic-list-cpp-typeinfo -Wl,--dynamic-list-data")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections -Wl,--no-export-dynamic -Bsymbolic-functions -Wl,--dynamic-list-cpp-typeinfo -Wl,--dynamic-list-data")
  endif()
endif()

include(CheckCXXCompilerFlag)

if(ICINGA2_LTO_BUILD)
  check_cxx_compiler_flag("-flto" CXX_FLAG_LTO)

  if(NOT CXX_FLAG_LTO)
    message(WARNING "Compiler does not support LTO, falling back to non-LTO build")
  else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -flto")
  endif()
endif()

include(CheckCCompilerFlag)

check_c_compiler_flag(-fvisibility-inlines-hidden HAVE_VISIBILITY_INLINES_HIDDEN)

if(HAVE_VISIBILITY_INLINES_HIDDEN)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility-inlines-hidden")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility-inlines-hidden")
endif()

check_c_compiler_flag(-fvisibility=hidden HAVE_VISIBILITY_HIDDEN)

if(HAVE_VISIBILITY_HIDDEN)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
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
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  exec_program(${CMAKE_CXX_COMPILER}
    ARGS -dumpversion
    OUTPUT_VARIABLE _ICINGA2_COMPILER_VERSION
  )

  if("${CMAKE_CXX_COMPILER_VERSION}" VERSION_LESS "4.6.0")
    message(FATAL_ERROR "Your version of GCC (${CMAKE_CXX_COMPILER_VERSION}) is too old for building Icinga 2 (GCC >= 4.6.0 is required).")
  endif()
endif()

if(NOT MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

  check_cxx_source_compiles("class Base { public: virtual void test(void) { } }; class Derived : public Base { virtual void test(void) override { } }; int main(){}" CXX_FEATURE_OVERRIDE)

  if(NOT CXX_FEATURE_OVERRIDE)
    add_definitions("-Doverride=")
  endif()
endif()

configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h ESCAPE_QUOTES)

install(
  FILES README.md COPYING COPYING.Exceptions AUTHORS ChangeLog NEWS
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

if(ICINGA2_WITH_STUDIO)
  add_subdirectory(icinga-studio)
endif()

if(ICINGA2_WITH_TESTS)
  add_subdirectory(test)
endif()

set(CPACK_PACKAGE_NAME "Icinga 2")
set(CPACK_PACKAGE_VENDOR "Icinga Development Team")
set(CPACK_PACKAGE_VERSION ${ICINGA2_VERSION})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "ICINGA2")
set(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/icinga-app\\\\icinga.ico")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_BINARY_DIR}/LICENSE.txt")

set(CPACK_PACKAGE_EXECUTABLES "Icinga2SetupAgent;Icinga 2 Agent Wizard;icinga-studio;Icinga Studio")
set(CPACK_WIX_PRODUCT_ICON "${CMAKE_CURRENT_SOURCE_DIR}/icinga-app\\\\icinga.ico")
set(CPACK_WIX_UPGRADE_GUID "52F2BEAA-4DF0-4C3E-ABDC-C0F61DE4DF8A")
set(CPACK_WIX_EXTENSIONS "WixUtilExtension")
set(CPACK_WIX_UI_BANNER "${CMAKE_CURRENT_SOURCE_DIR}/icinga-installer/bannrbmp.bmp")
set(CPACK_WIX_UI_DIALOG "${CMAKE_CURRENT_SOURCE_DIR}/icinga-installer/dlgbmp.bmp")
set(CPACK_WIX_PATCH_FILE "${CMAKE_CURRENT_BINARY_DIR}/icinga-installer/icinga2.wixpatch.Debug")
set(CPACK_WIX_PATCH_FILE "${CMAKE_CURRENT_BINARY_DIR}/icinga-installer/icinga2.wixpatch")

set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
include(InstallRequiredSystemLibraries)

if(WIN32)
  if(CMAKE_VS_PLATFORM_NAME STREQUAL "x64")
    set(NSCP_URL "https://github.com/mickem/nscp/releases/download/0.5.0.62/NSCP-0.5.0.62-x64.msi")
    set(NSCP_SHA256 "1854de86ad4fda3391f273de0f9985b702c014bdec01b26ad28a1343177f537f")
  else()
    set(NSCP_URL "https://github.com/mickem/nscp/releases/download/0.5.0.62/NSCP-0.5.0.62-Win32.msi")
    set(NSCP_SHA256 "2186b60d588fa0811344ce709332f9c63670019c62ae92eae49698bf76205a95")
  endif()

  set(NSCP_SHA256SUM "")
  if (EXISTS ${CMAKE_CURRENT_BINARY_DIR}/NSCP.msi)
    file(SHA256 ${CMAKE_CURRENT_BINARY_DIR}/NSCP.msi NSCP_SHA256SUM)
  endif()
    
  if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/NSCP.msi OR NOT ${NSCP_SHA256SUM} STREQUAL ${NSCP_SHA256})
    file(DOWNLOAD ${NSCP_URL} ${CMAKE_CURRENT_BINARY_DIR}/NSCP.msi SHOW_PROGRESS)
  endif()

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/NSCP.msi DESTINATION ${CMAKE_INSTALL_SBINDIR})

  install(
    PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
      ${OPENSSL_INCLUDE_DIR}/../bin/libeay32.dll ${OPENSSL_INCLUDE_DIR}/../bin/ssleay32.dll
    DESTINATION ${CMAKE_INSTALL_SBINDIR}
  )
endif()

include(CPack)
