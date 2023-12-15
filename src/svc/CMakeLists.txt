add_library(svc)
target_sources(svc
	PRIVATE
		principals.cpp
	PUBLIC
		FILE_SET headers TYPE HEADERS
		FILES
			principals.h
)

target_link_libraries(svc
	PUBLIC
		${PROJECT_NAME}::db
		${PROJECT_NAME}::libproto
)

if (SENTIUM_ENABLE_COVERAGE)
	target_compile_options(svc
		PRIVATE -fprofile-instr-generate -fcoverage-mapping
	)

	target_link_options(svc
		INTERFACE -fprofile-instr-generate
	)
endif()

# tests
if (SENTIUM_BUILD_TESTING)
	add_executable(svc_tests)
	target_sources(svc_tests
		PRIVATE
			principals_test.cpp
	)

	target_link_libraries(svc_tests
		PRIVATE
			svc
			gtest_main
	)

	if (SENTIUM_ENABLE_COVERAGE)
		target_compile_options(svc_tests
			PRIVATE -fprofile-instr-generate -fcoverage-mapping
		)

		target_link_options(svc_tests
			INTERFACE -fprofile-instr-generate
		)
	endif()

	include(GoogleTest)
	gtest_discover_tests(svc_tests)
endif()
