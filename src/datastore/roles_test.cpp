#include <gtest/gtest.h>

#include "permissions.h"
#include "roles.h"
#include "testing.h"

class RolesTest : public ::testing::Test {
protected:
	static void SetUpTestSuite() {
		datastore::testing::setup();

		// Clear data
		datastore::pg::exec("truncate table roles cascade;");
	}

	void SetUp() {
		// Clear data before each test
		datastore::pg::exec("delete from roles cascade;");
		datastore::pg::exec("delete from permissions cascade;");
	}

	static void TearDownTestSuite() { datastore::testing::teardown(); }
};

TEST_F(RolesTest, retrieve) {
	// Success: retrieve data
	{
		std::string_view qry = R"(
			insert into roles (
				_id,
				_rev,
				name
			) values (
				$1::text,
				$2::integer,
				$3::text
			);
		)";

		try {
			datastore::pg::exec(qry, "_id:RolesTest.retrieve", 2306, "name:RolesTest.retrieve");
		} catch (const std::exception &e) {
			FAIL() << e.what();
		}

		auto role = datastore::RetrieveRole("_id:RolesTest.retrieve");
		EXPECT_EQ(2306, role.rev());
		EXPECT_EQ("name:RolesTest.retrieve", role.name());
	}
}

TEST_F(RolesTest, store) {
	const datastore::Role role({
		.name = "name:RolesTest.store",
	});
	ASSERT_NO_THROW(role.store());

	std::string_view qry = R"(
		select
			_rev,
			name
		from roles
		where
			_id = $1::text;
	)";

	auto res = datastore::pg::exec(qry, role.id());
	ASSERT_EQ(1, res.size());

	auto [_rev, name] = res[0].as<int, std::string>();
	EXPECT_EQ(role.rev(), _rev);
	EXPECT_EQ(role.name(), name);
}

TEST_F(RolesTest, addPermission) {
	// Success: add permission
	{
		const datastore::Permission permission({
			.id = "permission:RolesTest.addPermission",
		});
		ASSERT_NO_THROW(permission.store());

		const datastore::Role role({
			.name = "name:RolesTest.addPermission",
		});
		ASSERT_NO_THROW(role.store());
		ASSERT_NO_THROW(role.addPermission(permission.id()));

		std::string_view qry = R"(
			select
				permission_id
			from "roles_permissions"
			where
				role_id = $1::text;
		)";

		auto res = datastore::pg::exec(qry, role.id());
		ASSERT_EQ(1, res.size());

		auto [permissionId] = res[0].as<std::string>();
		EXPECT_EQ(permission.id(), permissionId);
	}

	// Error: invalid permission
	{
		const datastore::Role role({
			.name = "name:RolesTest.addPermission-invalid_permission",
		});
		ASSERT_NO_THROW(role.store());

		ASSERT_THROW(role.addPermission("invalid"), err::DatastoreInvalidRoleOrPermission);
	}
}
