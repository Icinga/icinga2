cmake_minimum_required (VERSION 3.5.1)
project(spawn)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# requires boost 1.66, but may be provided by a parent project
if(NOT Boost_FOUND OR NOT Boost_system_FOUND OR NOT Boost_context_FOUND OR Boost_VERSION VERSION_LESS 1.66.0)
	find_package(Boost 1.66 REQUIRED COMPONENTS system context)
endif()

add_library(spawn INTERFACE)
target_include_directories(spawn INTERFACE include)
target_link_libraries(spawn INTERFACE Boost::system Boost::context)

option(SPAWN_INSTALL "install spawn headers" ON)
if(SPAWN_INSTALL)
	install(DIRECTORY include/spawn DESTINATION include)
endif()

option(SPAWN_TEST_ADDRESS_SANITIZER "build spawn tests with address sanitizer" ON)

option(SPAWN_BUILD_TESTS "build spawn tests" ON)
if(SPAWN_BUILD_TESTS)
	enable_testing()
	add_subdirectory(test)
endif()
