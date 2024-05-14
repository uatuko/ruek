cmake_minimum_required(VERSION 3.23)
project(sentium VERSION 0.2.0 LANGUAGES CXX)

cmake_policy(SET CMP0135 NEW)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

option(SENTIUM_BUILD_BENCHMARKS
	"Build benchmarks" OFF
)

include(CMakeDependentOption)
cmake_dependent_option(SENTIUM_BUILD_TESTING
	"Build tests when this is the root project" ON
	"CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR" OFF
)

cmake_dependent_option(SENTIUM_ENABLE_COVERAGE
	"Enable code coverage when SENTIUM_BUILD_TESTING is enabled and this is the root project" ON
	"SENTIUM_BUILD_TESTING;CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR" OFF
)

if (SENTIUM_ENABLE_COVERAGE)
	if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		message(FATAL_ERROR
			"Code coverage is only available with clang. "
			"Can't continue with SENTIUM_ENABLE_COVERAGE=ON."
		)
	endif()
endif()

include(cmake/dependencies.cmake)
include(cmake/googleapis.cmake)

if (SENTIUM_BUILD_TESTING)
	enable_testing()
endif()

if (SENTIUM_BUILD_BENCHMARKS)
	add_subdirectory(bench)
endif()

add_subdirectory(proto)
add_subdirectory(src)
