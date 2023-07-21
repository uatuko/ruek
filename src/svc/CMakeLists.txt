add_library(svc
	access.cpp access.h
	collections.cpp collections.h
	events.cpp events.h
	identities.cpp identities.h
	rbac.cpp rbac.h
	roles.cpp roles.h
	svc.h
	interceptors/logger.cpp interceptors/logger.h
)

target_link_libraries(svc
	PUBLIC
		${PROJECT_NAME}::libproto
	PRIVATE
		${PROJECT_NAME}::datastore
		${PROJECT_NAME}::err
		${PROJECT_NAME}::logger
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
		collections_test.cpp
		events_test.cpp
		identities_test.cpp
		rbac_test.cpp
		roles_test.cpp
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
