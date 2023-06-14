add_library(datastore
	collections.cpp collections.h
	datastore.h
	identities.cpp identities.h
	pg.cpp pg.h
	testing.h
)

target_link_libraries(datastore
	PUBLIC
		${PROJECT_NAME}::err
		pqxx
	PRIVATE
		libxid::xid
)

if (GATEKEEPER_ENABLE_COVERAGE)
	target_compile_options(datastore
		PRIVATE -fprofile-instr-generate -fcoverage-mapping
	)

	target_link_options(datastore
		INTERFACE -fprofile-instr-generate
	)
endif()

# tests
if (GATEKEEPER_ENABLE_TESTING)
	add_executable(datastore_tests
		collections_test.cpp
		identities_test.cpp
	)

	target_link_libraries(datastore_tests
		PRIVATE
			datastore
			gtest_main
	)

	if (GATEKEEPER_ENABLE_COVERAGE)
		target_compile_options(datastore_tests
			PRIVATE -fprofile-instr-generate -fcoverage-mapping
		)

		target_link_options(datastore_tests
			PRIVATE -fprofile-instr-generate
		)
	endif()

	include(GoogleTest)
	gtest_discover_tests(datastore_tests)
endif()

