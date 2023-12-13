add_library(db)
target_sources(db
	PRIVATE
		pg.cpp
	PUBLIC
		FILE_SET headers TYPE HEADERS
		FILES
			config.h
			pg.h
)

target_link_libraries(db
	PRIVATE
		${PROJECT_NAME}::err
	PUBLIC
		pqxx
)

if (SENTIUM_ENABLE_COVERAGE)
	target_compile_options(db
		PRIVATE -fprofile-instr-generate -fcoverage-mapping
	)

	target_link_options(db
		INTERFACE -fprofile-instr-generate
	)
endif()

# tests
if (SENTIUM_BUILD_TESTING)
	target_sources(db
		PUBLIC
			FILE_SET headers TYPE HEADERS
			FILES
				testing.h
	)

	add_executable(db_tests)
	target_sources(db_tests
		PRIVATE
			pg_test.cpp
	)

	target_link_libraries(db_tests
		PRIVATE
			db
			gtest_main
			${PROJECT_NAME}::err
	)

	if (SENTIUM_ENABLE_COVERAGE)
		target_compile_options(db_tests
			PRIVATE -fprofile-instr-generate -fcoverage-mapping
		)

		target_link_options(db_tests
			INTERFACE -fprofile-instr-generate
		)
	endif()

	include(GoogleTest)
	gtest_discover_tests(db_tests)
endif()
