add_library(service
	gatekeeper.access-policies.cpp
	gatekeeper.collections.cpp
	gatekeeper.cpp gatekeeper.h
	gatekeeper.identities.cpp
	gatekeeper.rbac-policies.cpp
	gatekeeper.roles.cpp
	mappers.cpp mappers.h
	interceptors/logger.cpp interceptors/logger.h
)

target_link_libraries(service
	PUBLIC
		${PROJECT_NAME}::libproto
	PRIVATE
		${PROJECT_NAME}::datastore
		${PROJECT_NAME}::logger
)

if (GATEKEEPER_ENABLE_COVERAGE)
	target_compile_options(service
		PRIVATE -fprofile-instr-generate -fcoverage-mapping
	)

	target_link_options(service
		INTERFACE -fprofile-instr-generate
	)
endif()

# tests
if (GATEKEEPER_ENABLE_TESTING)
	add_executable(service_tests
		gatekeeper.access-policies_test.cpp
		gatekeeper.collections_test.cpp
		gatekeeper_test.cpp
		gatekeeper.identities_test.cpp
		gatekeeper.rbac-policies_test.cpp
		gatekeeper.roles_test.cpp
	)

	target_link_libraries(service_tests
		PRIVATE
			service
			gtest_main
			${PROJECT_NAME}::datastore
	)

	if (GATEKEEPER_ENABLE_COVERAGE)
		target_compile_options(service_tests
			PRIVATE -fprofile-instr-generate -fcoverage-mapping
		)

		target_link_options(service_tests
			PRIVATE -fprofile-instr-generate
		)
	endif()

	include(GoogleTest)
	gtest_discover_tests(service_tests)
endif()
