cmake_minimum_required(VERSION 3.21)
project(gatekeeper VERSION 0.0.1 LANGUAGES CXX)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BUILD_SHARED_LIBS OFF)

option(GATEKEEPER_ENABLE_COVERAGE "Enable code coverage" OFF)
option(GATEKEEPER_ENABLE_TESTING "Enable testing" OFF)
option(GATEKEEPER_FAVOUR_SYSTEM_GRPC "Favour system installed gRPC over building from source" ON)

if (GATEKEEPER_ENABLE_COVERAGE)
	if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		message(FATAL_ERROR
			"Code coverage is only available with clang. "
			"Can't continue with GATEKEEPER_ENABLE_COVERAGE=ON."
		)
	endif()
endif()

include(cmake/dependencies.cmake)
include(cmake/googleapis.cmake)

if (GATEKEEPER_ENABLE_TESTING)
	enable_testing()

	cmake_path(SET gatekeeper_test_conf_path ${CMAKE_CURRENT_SOURCE_DIR}/conf)
	add_compile_definitions(GATEKEEPER_TEST_CONF_PATH="${gatekeeper_test_conf_path}")
endif()

add_subdirectory(proto)
add_subdirectory(src)
