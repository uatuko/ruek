#include <gtest/gtest.h>

#include "err/errors.h"

#include "rbac-policies.h"
#include "identities.h"
#include "testing.h"

class RbacPoliciesTest : public ::testing::Test {
protected:
	static void SetUpTestSuite() {
		datastore::testing::setup();

		// Clear data
		datastore::pg::exec(R"(truncate table "rbac-policies" cascade;)");
	}

	void SetUp() {
		// Clear data before each test
		datastore::pg::exec(R"(delete from "rbac-policies" cascade;)");
	}

	static void TearDownTestSuite() { datastore::testing::teardown(); }
};

TEST_F(RbacPoliciesTest, principals) {
	// Success: add principals
	{
		const datastore::RbacPolicy policy({
			.name = "name:RbacPoliciesTest.principals-add",
		});
		EXPECT_NO_THROW(policy.store());

		const datastore::Identity identity({
			.sub = "sub:RbacPoliciesTest.principals-add",
		});
		EXPECT_NO_THROW(identity.store());

		EXPECT_NO_THROW(policy.addPrincipal(identity.id()));

		std::string_view qry = R"(
			select
				count(*) as n_principals
			from "rbac-policies_identities"
			where
				policy_id = $1::text and
				identity_id = $2::text;
		)";

		auto res = datastore::pg::exec(qry, policy.id(), identity.id());
		EXPECT_EQ(1, res.at(0, 0).as<int>());

		// cleanup
		EXPECT_NO_THROW(identity.discard());
	}

  // Error: add invalid principals
	{
		const datastore::RbacPolicy policy({
			.name = "name:RbacPoliciesTest.principals-add_invalid-principal",
		});
		EXPECT_NO_THROW(policy.store());

		EXPECT_THROW(policy.addPrincipal("invalid-principal"), err::DatastoreInvalidRbacPolicyOrPrincipal);
	}

	// Error: add principal to invalid policy
	{
		const datastore::RbacPolicy policy({
			.name = "name:RbacPoliciesTest.principals-add_invalid-policy",
		});

		const datastore::Identity identity({
			.sub = "sub:RbacPoliciesTest.principals-add_invalid-policy",
		});
		EXPECT_NO_THROW(identity.store());

		EXPECT_THROW(policy.addPrincipal(identity.id()), err::DatastoreInvalidRbacPolicyOrPrincipal);
	}

	// Error: duplicate principal
	{
		const datastore::RbacPolicy policy({
			.name = "name:RbacPoliciesTest.principals-add_duplicate-principal",
		});
		EXPECT_NO_THROW(policy.store());

		const datastore::Identity identity({
			.sub = "sub:RbacPoliciesTest.principals-add_duplicate-principal",
		});
		EXPECT_NO_THROW(identity.store());

		EXPECT_NO_THROW(policy.addPrincipal(identity.id()));
		EXPECT_THROW(policy.addPrincipal(identity.id()), err::DatastoreDuplicateRbacPolicyPrincipal);
	}

	// Success: list principals
	{
		const datastore::RbacPolicies policies({
			{{.name = "name:RbacPoliciesTest.principals-list[0]"}},
			{{.name = "name:RbacPoliciesTest.principals-list[1]"}},
		});

		for (const auto &policy : policies) {
			EXPECT_NO_THROW(policy.store());
		}

		const datastore::Identities identities({
			{{.sub = "sub:RbacPoliciesTest.principals-list[0]"}},
			{{.sub = "sub:RbacPoliciesTest.principals-list[1]"}},
		});

		for (const auto &identity : identities) {
			EXPECT_NO_THROW(identity.store());
		}

		EXPECT_NO_THROW(policies[0].addPrincipal(identities[0].id()));
		EXPECT_NO_THROW(policies[0].addPrincipal(identities[1].id()));

		{
			auto principals = policies[0].principals();
			EXPECT_EQ(2, principals.size());

			const datastore::RbacPolicy::principals_t expected = {
				identities[0].id(), identities[1].id()};
			EXPECT_EQ(expected, principals);
		}
		{
			auto principals = policies[1].principals();
			EXPECT_EQ(0, principals.size());
		}

		// cleanup
		for (const auto &identity : identities) {
			EXPECT_NO_THROW(identity.discard());
		}
	}
}

TEST_F(RbacPoliciesTest, store) {
	const datastore::RbacPolicy policy({
		.name = "name:RbacPoliciesTest.store",
	});

	EXPECT_NO_THROW(policy.store());

	std::string_view qry = R"(
		select
			_rev,
			name
		from "rbac-policies"
		where _id = $1::text;
	)";

	auto res = datastore::pg::exec(qry, policy.id());
	EXPECT_EQ(1, res.size());

	auto [_rev, name] = res[0].as<int, std::string>();
	EXPECT_EQ(policy.rev(), _rev);
	EXPECT_EQ(policy.name(), name);
}