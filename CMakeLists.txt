cmake_minimum_required(VERSION 3.23)
project(ruek VERSION 0.3.1 LANGUAGES CXX)

cmake_policy(SET CMP0135 NEW)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)
option(RUEK_BUILD_BENCHMARKS "Build benchmarks" OFF)

include(CMakeDependentOption)
cmake_dependent_option(RUEK_BUILD_TESTING
	"Build tests when BUILD_TESTING flag is set or this is the root project" ON
	"BUILD_TESTING OR CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR" OFF
)

cmake_dependent_option(RUEK_ENABLE_COVERAGE
	"Enable code coverage when RUEK_BUILD_TESTING is enabled and this is the root project" ON
	"NOT BUILD_SHARED_LIBS; RUEK_BUILD_TESTING; CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR" OFF
)

if (RUEK_ENABLE_COVERAGE)
	if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		message(FATAL_ERROR
			"Code coverage is only available with clang. "
			"Can't continue with RUEK_ENABLE_COVERAGE=ON."
		)
	endif()
endif()

include(cmake/dependencies.cmake)

if (RUEK_BUILD_TESTING)
	enable_testing()
endif()

if (RUEK_BUILD_BENCHMARKS)
	add_subdirectory(bench)
endif()

add_subdirectory(proto)
add_subdirectory(src)
