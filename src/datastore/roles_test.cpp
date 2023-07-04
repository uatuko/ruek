#include <gtest/gtest.h>

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
				name,
				permissions
			) values (
				$1::text,
				$2::integer,
				$3::text,
				$4::text[]
			);
		)";

		try {
			datastore::pg::exec(
				qry,
				"_id:RolesTest.retrieve",
				2306,
				"name:RolesTest.retrieve",
				R"({"permissions[0]:RolesTest.retrieve"})");
		} catch (const std::exception &e) {
			FAIL() << e.what();
		}

		auto role = datastore::RetrieveRole("_id:RolesTest.retrieve");
		EXPECT_EQ(2306, role.rev());
		EXPECT_EQ("name:RolesTest.retrieve", role.name());

		const auto &perms = role.permissions();
		EXPECT_EQ(1, perms.size());
		EXPECT_EQ("permissions[0]:RolesTest.retrieve", *perms.cbegin());
	}
}

TEST_F(RolesTest, store) {
	const datastore::Role role({
		.name = "name:RolesTest.store",
		.permissions =
			{
				{"permissions[0]:RolesTest.store"},
				{"permissions[1]:RolesTest.store"},
			},
	});
	ASSERT_NO_THROW(role.store());

	std::string_view qry = R"(
		select
			_rev,
			name,
			permissions
		from roles
		where
			_id = $1::text;
	)";

	auto res = datastore::pg::exec(qry, role.id());
	ASSERT_EQ(1, res.size());

	auto [_rev, name, permissions] = res[0].as<int, std::string, std::string>();
	EXPECT_EQ(role.rev(), _rev);
	EXPECT_EQ(role.name(), name);
	EXPECT_EQ(R"({permissions[0]:RolesTest.store,permissions[1]:RolesTest.store})", permissions);
}
