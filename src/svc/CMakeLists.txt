add_library(svc
	access.cpp access.h
	events.cpp events.h
)

target_link_libraries(svc
	PUBLIC
		${PROJECT_NAME}::libproto
	PRIVATE
		${PROJECT_NAME}::datastore
		${PROJECT_NAME}::err
)

if (GATEKEEPER_ENABLE_COVERAGE)
	target_compile_options(svc
		PRIVATE -fprofile-instr-generate -fcoverage-mapping
	)

	target_link_options(svc
		INTERFACE -fprofile-instr-generate
	)
endif()

# tests
if (GATEKEEPER_ENABLE_TESTING)
	add_executable(svc_tests
		access_test.cpp
		events_test.cpp
	)

	target_link_libraries(svc_tests
		PRIVATE
			svc
			gtest_main
			${PROJECT_NAME}::datastore
	)

	if (GATEKEEPER_ENABLE_COVERAGE)
		target_compile_options(svc_tests
			PRIVATE -fprofile-instr-generate -fcoverage-mapping
		)

		target_link_options(svc_tests
			PRIVATE -fprofile-instr-generate
		)
	endif()

	include(GoogleTest)
	gtest_discover_tests(svc_tests)
endif()
