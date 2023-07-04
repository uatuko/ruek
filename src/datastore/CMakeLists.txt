add_library(datastore
	access-policies.cpp access-policies.h
	collections.cpp collections.h
	config.h
	datastore.h
	identities.cpp identities.h
	pg.cpp pg.h
	redis.cpp redis.h
	roles.cpp roles.h
)

target_link_libraries(datastore
	PUBLIC
		${PROJECT_NAME}::err
		hiredis::hiredis_static
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
	target_sources(datastore
		PRIVATE
			testing.h
	)

	target_link_libraries(datastore
		PUBLIC
			libviper::viper
	)

	add_executable(datastore_tests
		access-policies_test.cpp
		collections_test.cpp
		identities_test.cpp
		redis_test.cpp
		roles_test.cpp
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

