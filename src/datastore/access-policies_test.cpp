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
			.name = "name:AccessPoliciesTest.add",
		});
		EXPECT_NO_THROW(policy.store());

		EXPECT_NO_THROW(datastore::RetrieveAccessPolicy(policy.id()));

		// cleanup
		EXPECT_NO_THROW(policy.discard());
	}

	// Success: add an access entry to a policy
	{
		const datastore::AccessPolicy policy({
			.name = "name:AccessPoliciesTest.add-access",
		});
		EXPECT_NO_THROW(policy.store());

		// store access
		auto record = datastore::AccessPolicy::Record(
			"principal-id:AccessPoliciesTest.add-access",
			"resource-id:AccessPoliciesTest.add-access");
		EXPECT_NO_THROW(policy.add(record));

		EXPECT_EQ(datastore::CheckAccess(record).size(), 1);

		// cleanup
		EXPECT_NO_THROW(policy.discard());
		EXPECT_NO_THROW(datastore::DeleteAccess(record));
	}
}
