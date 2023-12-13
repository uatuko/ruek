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

	include(GoogleTest)
	gtest_discover_tests(db_tests)
endif()
