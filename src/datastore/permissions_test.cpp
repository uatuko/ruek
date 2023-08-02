#include <gtest/gtest.h>

#include "permissions.h"
#include "roles.h"
#include "testing.h"

class PermissionsTest : public ::testing::Test {
protected:
	static void SetUpTestSuite() {
		datastore::testing::setup();

		// Clear data
		datastore::pg::exec("truncate table permissions cascade;");
	}

	void SetUp() {
		// Clear data before each test
		datastore::pg::exec("delete from permissions cascade;");
	}

	static void TearDownTestSuite() { datastore::testing::teardown(); }
};

TEST_F(PermissionsTest, retrieve) {
	// Success: retrieve data
	{
		std::string_view qry = R"(
			insert into permissions (
				_id
			) values (
				$1::text
			);
		)";

		try {
			datastore::pg::exec(qry, "_id:PermissionsTest.retrieve");
		} catch (const std::exception &e) {
			FAIL() << e.what();
		}

		auto permission = datastore::RetrievePermission("_id:PermissionsTest.retrieve");
		EXPECT_EQ("_id:PermissionsTest.retrieve", permission.id());
	}
}

TEST_F(PermissionsTest, store) {
	const datastore::Permission permission({
		.id = "id:PermissionsTest.store",
	});
	ASSERT_NO_THROW(permission.store());

	std::string_view qry = R"(
		select
			_id
		from permissions
		where
			_id = $1::text;
	)";

	auto res = datastore::pg::exec(qry, permission.id());
	ASSERT_EQ(1, res.size());

	auto [id] = res[0].as<std::string>();
	EXPECT_EQ(permission.id(), id);
}
