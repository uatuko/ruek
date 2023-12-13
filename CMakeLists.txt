cmake_minimum_required(VERSION 3.23)
project(sentium VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

include(CMakeDependentOption)
cmake_dependent_option(SENTIUM_BUILD_TESTING
	"Build tests when this is the root project" ON
	"CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR" OFF
)

include(cmake/dependencies.cmake)
include(cmake/googleapis.cmake)

if (SENTIUM_BUILD_TESTING)
	enable_testing()
endif()

add_subdirectory(proto)
add_subdirectory(src)
