add_library(events
	basic_event.h
	concepts.h
	events.cpp events.h
	publisher.cpp publisher.h
)

target_link_libraries(events
	PUBLIC
		glaze::glaze
)

# tests
if (GATEKEEPER_ENABLE_TESTING)
	add_executable(events_tests
		events_test.cpp
	)

	target_link_libraries(events_tests
		PRIVATE
			events
			gtest_main
	)

	if (GATEKEEPER_ENABLE_COVERAGE)
		target_compile_options(events_tests
			PRIVATE -fprofile-instr-generate -fcoverage-mapping
		)

		target_link_options(events_tests
			PRIVATE -fprofile-instr-generate
		)
	endif()

	include(GoogleTest)
	gtest_discover_tests(events_tests)
endif()
