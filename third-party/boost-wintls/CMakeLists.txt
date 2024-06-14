cmake_minimum_required(VERSION 3.15)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

include(VersionFromGit)

version_from_git()

project(boost-wintls
  VERSION ${VERSION}
  DESCRIPTION "Native Windows TLS stream for Boost.Asio"
  HOMEPAGE_URL "https://wintls.dev/"
  LANGUAGES CXX
)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(STATUS "Building for 64 bit platform")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
  message(STATUS "Building for 32 bit platform")
endif()

option(ENABLE_TESTING "Enable Test Builds" ${WIN32})
option(ENABLE_EXAMPLES "Enable Examples Builds" ${WIN32})
option(ENABLE_DOCUMENTATION "Enable Documentation Builds" ${UNIX})
option(ENABLE_ADDRESS_SANITIZER "Enable Address Sanitizer" OFF)
option(WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME}
  INTERFACE
  $<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_14)

if(WIN32)
  set(Boost_USE_STATIC_LIBS ON)
  set(Boost_USE_MULTITHREADED ON)
  set(Boost_USE_STATIC_RUNTIME OFF)

  target_compile_definitions(${PROJECT_NAME} INTERFACE
    BOOST_ALL_NO_LIB        # Disable auto linking boost libraries
    _CRT_SECURE_NO_WARNINGS # Ignore silly warnings on not using MS specific "secure" C functions
    _WIN32_WINNT=0x0601     # Target Windows 7
  )

  # Based on https://github.com/lefticus/cpp_starter_project
  set(MSVC_WARNINGS
    /W4 # Baseline reasonable warnings
    /w14254 # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits', possible loss of data
    /w14263 # 'function': member function does not override any base class virtual member function
    /w14287 # 'operator': unsigned/negative constant mismatch
    /we4289 # nonstandard extension used: 'variable': loop control variable declared in the for-loop is used outside the for-loop scope
    /w14296 # 'operator': expression is always 'boolean_value'
    /w14311 # 'variable': pointer truncation from 'type1' to 'type2'
    /w14545 # expression before comma evaluates to a function which is missing an argument list
    /w14546 # function call before comma missing argument list
    /w14547 # 'operator': operator before comma has no effect; expected operator with side-effect
    /w14549 # 'operator': operator before comma has no effect; did you intend 'operator'?
    /w14555 # expression has no effect; expected expression with side- effect
    /w14640 # Enable warning on thread un-safe static member initialization
    /w14826 # Conversion from 'type1' to 'type_2' is sign-extended. This may cause unexpected runtime behavior.
    /w14905 # wide string literal cast to 'LPSTR'
    /w14906 # string literal cast to 'LPWSTR'
    /w14928 # illegal copy-initialization; more than one user-defined conversion has been implicitly applied
    /w15038 # data member 'member1' will be initialized after data member 'member2'
    /w14062 # enumerator 'identifier' in switch of enum 'enumeration' is not handled
    /permissive- # standards conformance mode for MSVC compiler.
  )

  if(WARNINGS_AS_ERRORS)
    set(CLANG_WARNINGS ${CLANG_WARNINGS} -Werror)
    set(MSVC_WARNINGS ${MSVC_WARNINGS} /WX)
  endif()

  set(CLANG_WARNINGS
    -Wall
    -Wextra # reasonable and standard
    -Wshadow # warn the user if a variable declaration shadows one from a parent context
    -Wnon-virtual-dtor # warn the user if a class with virtual functions has a non-virtual destructor. This helps catch hard to track down memory errors
    -Wold-style-cast # warn for c-style casts
    -Wcast-align # warn for potential performance problem casts
    -Wunused # warn on anything being unused
    -Woverloaded-virtual # warn if you overload (not override) a virtual function
    -Wpedantic # warn if non-standard C++ is used
    -Wconversion # warn on type conversions that may lose data
    -Wsign-conversion # warn on sign conversions
    -Wnull-dereference # warn if a null dereference is detected
    -Wdouble-promotion # warn if float is implicit promoted to double
    -Wformat=2 # warn on security issues around functions that format output (ie printf)
    -Wimplicit-fallthrough # warn on statements that fallthrough without an explicit annotation
  )

  set(GCC_WARNINGS
    ${CLANG_WARNINGS}
    -Wmisleading-indentation # warn if indentation implies blocks where blocks do not exist
    -Wduplicated-cond # warn if if / else chain has duplicated conditions
    -Wduplicated-branches # warn if if / else branches have duplicated code
    -Wlogical-op # warn about logical operations being used where bitwise were probably wanted
    -Wuseless-cast # warn if you perform a cast to the same type
  )

  if(MSVC)
    set(PROJECT_WARNINGS ${MSVC_WARNINGS})
  elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
    set(PROJECT_WARNINGS ${CLANG_WARNINGS})
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(PROJECT_WARNINGS ${GCC_WARNINGS})
  else()
    message(AUTHOR_WARNING "No compiler warnings set for '${CMAKE_CXX_COMPILER_ID}' compiler.")
  endif()

  target_compile_options(${PROJECT_NAME} INTERFACE ${PROJECT_WARNINGS})

  # Generate .pdb files with debug info for release builds
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
endif()

find_package(Boost REQUIRED)

target_link_libraries(${PROJECT_NAME} INTERFACE
  Boost::headers
)

if(MINGW)
  target_link_libraries(${PROJECT_NAME} INTERFACE
    crypt32
    secur32
    ws2_32
    wsock32
  )
endif()

if(ENABLE_ADDRESS_SANITIZER)
  message(STATUS "Enabling Address Sanitizer.")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /fsanitize=address")
  set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /incremental:no")
endif()

if(ENABLE_TESTING)
  enable_testing()
  message(STATUS "Building Tests.")
  add_subdirectory(test)
endif()

if(ENABLE_EXAMPLES)
  message(STATUS "Building Examples.")
  add_subdirectory(examples)
endif()

if(ENABLE_DOCUMENTATION)
  message(STATUS "Building Documentation.")
  add_subdirectory(doc)
endif()
set(CPACK_SOURCE_GENERATOR ZIP)
set(CPACK_SOURCE_PACKAGE_FILE_NAME ${PROJECT_NAME}-${PROJECT_VERSION})
set(CPACK_SOURCE_IGNORE_FILES
  "/\\\\..*$"
  "build"
  "__pycache__"
)
include(CPack)
include(ClangFormat)
