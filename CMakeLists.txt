cmake_minimum_required(VERSION 3.11)
project(gatekeeper VERSION 0.0.1 LANGUAGES CXX)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BUILD_SHARED_LIBS OFF)

option(GATEKEEPER_ENABLE_COVERAGE "Enable code coverage" OFF)
option(GATEKEEPER_ENABLE_TESTING "Enable testing" OFF)

include(cmake/dependencies.cmake)
include(cmake/googleapis.cmake)

if (GATEKEEPER_ENABLE_TESTING)
	enable_testing()
endif()

add_subdirectory(proto)
add_subdirectory(src)

