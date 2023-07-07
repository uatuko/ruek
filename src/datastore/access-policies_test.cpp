#include <gtest/gtest.h>

#include "err/errors.h"

#include "access-policies.h"
#include "collections.h"
#include "identities.h"
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
		datastore::pg::exec("delete from collections cascade;");
		datastore::pg::exec("delete from identities cascade;");
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

		EXPECT_EQ(record.check().size(), 1);

		// cleanup
		EXPECT_NO_THROW(policy.discard());
		EXPECT_NO_THROW(record.discard());
	}
}

TEST_F(AccessPoliciesTest, retrieveIdentities) {
	// Success: retrieve identities
	{
		const datastore::Identities identities({
			datastore::Identity::Data{.sub = "sub:AccessPoliciesTest.retrieveIdentities[0]"},
			datastore::Identity::Data{.sub = "sub:AccessPoliciesTest.retrieveIdentities[1]"},
		});

		for (const auto &idn : identities) {
			ASSERT_NO_THROW(idn.store());
		}

		const datastore::Collection collection({
			.name = "name:AccessPoliciesTest.retrieveIdentities",
		});
		ASSERT_NO_THROW(collection.store());
		ASSERT_NO_THROW(collection.add(identities[0].id()));

		const datastore::AccessPolicy policy({
			.name = "name:AccessPoliciesTest.retrieveIdentities",
		});
		ASSERT_NO_THROW(policy.store());

		ASSERT_NO_THROW(policy.addCollectionPrincipal(collection.id()));
		ASSERT_NO_THROW(policy.addIdentityPrincipal(identities[1].id()));

		const auto results = datastore::RetrieveAccessPolicyIdentities(policy.id());
		EXPECT_EQ(2, results.size());

		for (const auto &idn : identities) {
			EXPECT_TRUE(results.contains(idn.id()));
		}
	}
}
