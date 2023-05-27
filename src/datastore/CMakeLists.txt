add_library(datastore
	datastore.h
	identities.cpp
	identities.h
	pg.cpp
	pg.h
	testing.h
)

target_link_libraries(datastore
	PUBLIC
		${PROJECT_NAME}::err
		pqxx
	PRIVATE
		libxid::xid
)

# tests
add_executable(datastore_tests
	identities_test.cpp
)

target_link_libraries(datastore_tests
	PRIVATE
		datastore
		gtest_main
)

include(GoogleTest)
gtest_discover_tests(datastore_tests)

