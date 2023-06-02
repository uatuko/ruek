add_library(service
	grpc.cpp grpc.h
	mappers.cpp mappers.h
)

target_link_libraries(service
	PUBLIC
		${PROJECT_NAME}::libprotos
	PRIVATE
		${PROJECT_NAME}::datastore
)

if (GATEKEEPER_ENABLE_COVERAGE)
	target_compile_options(service
		PRIVATE --coverage
	)

	target_link_options(service
		INTERFACE -fprofile-arcs
	)
endif()

# test
if (GATEKEEPER_ENABLE_TESTING)
	add_executable(service_tests
		grpc_test.cpp
	)

	target_link_libraries(service_tests
		PRIVATE
			service
			gtest_main
			${PROJECT_NAME}::datastore
	)

	if (GATEKEEPER_ENABLE_COVERAGE)
		target_compile_options(service_tests
			PRIVATE --coverage
		)

		target_link_options(service_tests
			PRIVATE -fprofile-arcs
		)
	endif()

	include(GoogleTest)
	gtest_discover_tests(service_tests)
endif()
