add_library(err INTERFACE)

target_sources(err
	INTERFACE
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/basic_error.h>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/errors.h>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/fixed_string.h>
)

# tests
if (RUEK_BUILD_TESTING)
	add_executable(err_tests)
	target_sources(err_tests
		PRIVATE
			basic_error_test.cpp
	)

	target_link_libraries(err_tests
		PRIVATE
			err
			GTest::gtest_main
	)

	if (RUEK_ENABLE_COVERAGE)
		target_compile_options(err_tests
			PRIVATE -fprofile-instr-generate -fcoverage-mapping
		)

		target_link_options(err_tests
			PRIVATE -fprofile-instr-generate
		)
	endif()

	include(GoogleTest)
	gtest_discover_tests(err_tests)
endif()
