add_library(encoding INTERFACE)

target_sources(encoding
	INTERFACE
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/b32.h>
)

# tests
if (SENTIUM_BUILD_TESTING)
	add_executable(encoding_tests)
	target_sources(encoding_tests
		PRIVATE
			b32_test.cpp
	)

	target_link_libraries(encoding_tests
		PRIVATE
			encoding
			gtest_main
	)

	if (SENTIUM_ENABLE_COVERAGE)
		target_compile_options(encoding_tests
			PRIVATE -fprofile-instr-generate -fcoverage-mapping
		)

		target_link_options(encoding_tests
			PRIVATE -fprofile-instr-generate
		)
	endif()

	include(GoogleTest)
	gtest_discover_tests(encoding_tests)
endif()
