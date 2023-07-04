#include <gtest/gtest.h>

#include "err/errors.h"

#include "access-policies.h"
#include "testing.h"

class AccessPoliciesTest : public ::testing::Test {
protected:
	static void SetUpTestSuite() {
		datastore::testing::setup();

		// Clear data
		datastore::pg::exec("truncate table \"access-policies\" cascade;");
	}

	void SetUp() {
		// Clear data before each test
		datastore::pg::exec("delete from \"access-policies\" cascade;");
	}

	static void TearDownTestSuite() { datastore::testing::teardown(); }
};

TEST_F(AccessPoliciesTest, create) {
	// Success: add policy
	{
		const datastore::AccessPolicy policy({
			.name = "name:AccessPoliciesTest.XXX",
		});
		EXPECT_NO_THROW(policy.store());

		EXPECT_NO_THROW(datastore::RetrieveAccessPolicy(policy.id()));

		// std::string_view qry = R"(
		// 	select
		// 		count(*) as n_members
		// 	from access-policies
		// 	where
		// 		collection_id = $1::text and
		// 		identity_id = $2::text;
		// )";

		// auto res = datastore::pg::exec(qry, collection.id(), identity.id());
		// EXPECT_EQ(1, res.at(0, 0).as<int>());

		// cleanup
		EXPECT_NO_THROW(policy.discard());
	}
}
