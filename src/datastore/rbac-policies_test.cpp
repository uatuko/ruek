#include <gtest/gtest.h>

#include "err/errors.h"

#include "collections.h"
#include "identities.h"
#include "rbac-policies.h"
#include "roles.h"
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
	// Success: add identity principal
	{
		const datastore::RbacPolicy policy({
			.name = "name:RbacPoliciesTest.principals-add-identity",
		});
		EXPECT_NO_THROW(policy.store());

		const datastore::Identity identity({
			.sub = "sub:RbacPoliciesTest.principals-add-identity",
		});
		EXPECT_NO_THROW(identity.store());

		EXPECT_NO_THROW(policy.addIdentity(identity.id()));

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

	// Success: add collection principal
	{
		const datastore::RbacPolicy policy({
			.name = "name:RbacPoliciesTest.principals-add-collection",
		});
		EXPECT_NO_THROW(policy.store());

		const datastore::Collection collection({
			.name = "name:RbacPoliciesTest.principals-add-collection",
		});
		EXPECT_NO_THROW(collection.store());

		EXPECT_NO_THROW(policy.addCollection(collection.id()));

		std::string_view qry = R"(
			select
				count(*) as n_principals
			from "rbac-policies_collections"
			where
				policy_id = $1::text and
				collection_id = $2::text;
		)";

		auto res = datastore::pg::exec(qry, policy.id(), collection.id());
		EXPECT_EQ(1, res.at(0, 0).as<int>());
	}

	// Error: add invalid identity principals
	{
		const datastore::RbacPolicy policy({
			.name = "name:RbacPoliciesTest.principals-add_invalid-principal-identity",
		});
		EXPECT_NO_THROW(policy.store());

		EXPECT_THROW(
			policy.addIdentity("invalid-principal-identity"),
			err::DatastoreInvalidRbacPolicyOrPrincipal);
	}

	// Error: add invalid collection principals
	{
		const datastore::RbacPolicy policy({
			.name = "name:RbacPoliciesTest.principals-add_invalid-principal-collection",
		});
		EXPECT_NO_THROW(policy.store());

		EXPECT_THROW(
			policy.addCollection("invalid-principal-collection"),
			err::DatastoreInvalidRbacPolicyOrPrincipal);
	}

	// Error: add identity principal to invalid policy
	{
		const datastore::RbacPolicy policy({
			.name = "name:RbacPoliciesTest.principals-add_invalid-policy-identity",
		});

		const datastore::Identity identity({
			.sub = "sub:RbacPoliciesTest.principals-add_invalid-policy-identity",
		});
		EXPECT_NO_THROW(identity.store());

		EXPECT_THROW(policy.addIdentity(identity.id()), err::DatastoreInvalidRbacPolicyOrPrincipal);
	}

	// Error: add collection principal to invalid policy
	{
		const datastore::RbacPolicy policy({
			.name = "name:RbacPoliciesTest.principals-add_invalid-policy-collection",
		});

		const datastore::Collection collection({
			.name = "name:RbacPoliciesTest.principals-add_invalid-policy-collection",
		});
		EXPECT_NO_THROW(collection.store());

		EXPECT_THROW(
			policy.addCollection(collection.id()), err::DatastoreInvalidRbacPolicyOrPrincipal);
	}

	// Error: duplicate identity principal
	{
		const datastore::RbacPolicy policy({
			.name = "name:RbacPoliciesTest.principals-add_duplicate-identity-principal",
		});
		EXPECT_NO_THROW(policy.store());

		const datastore::Identity identity({
			.sub = "sub:RbacPoliciesTest.principals-add_duplicate-identity-principal",
		});
		EXPECT_NO_THROW(identity.store());

		EXPECT_NO_THROW(policy.addIdentity(identity.id()));
		EXPECT_THROW(policy.addIdentity(identity.id()), err::DatastoreDuplicateRbacPolicyPrincipal);
	}

	// Error: duplicate collection principal
	{
		const datastore::RbacPolicy policy({
			.name = "name:RbacPoliciesTest.principals-add_duplicate-collection-principal",
		});
		EXPECT_NO_THROW(policy.store());

		const datastore::Collection collection({
			.name = "name:RbacPoliciesTest.principals-add_duplicate-collection-principal",
		});
		EXPECT_NO_THROW(collection.store());

		EXPECT_NO_THROW(policy.addCollection(collection.id()));
		EXPECT_THROW(
			policy.addCollection(collection.id()), err::DatastoreDuplicateRbacPolicyPrincipal);
	}
}

TEST_F(RbacPoliciesTest, roles) {
	// Success: add roles
	{
		const datastore::RbacPolicy policy({
			.name = "name:RbacPoliciesTest.roles-add",
		});
		EXPECT_NO_THROW(policy.store());

		const datastore::Role role({
			.name = "name:RbacPoliciesTest.roles-add",
		});
		EXPECT_NO_THROW(role.store());

		auto rule = datastore::RbacPolicy::Rule({.roleId = role.id()});

		EXPECT_NO_THROW(policy.addRule(rule));

		std::string_view qry = R"(
			select
				count(*) as n_roles
			from "rbac-policies_roles"
			where
				policy_id = $1::text and
				role_id = $2::text;
		)";

		auto res = datastore::pg::exec(qry, policy.id(), role.id());
		EXPECT_EQ(1, res.at(0, 0).as<int>());
	}

	// Error: add invalid role
	{
		const datastore::RbacPolicy policy({
			.name = "name:RbacPoliciesTest.roles-add_invalid-role",
		});
		EXPECT_NO_THROW(policy.store());

		auto rule = datastore::RbacPolicy::Rule({.roleId = "invalid-role"});

		EXPECT_THROW(policy.addRule(rule), err::DatastoreInvalidRbacPolicyOrRole);
	}

	// Error: add role to invalid policy
	{
		const datastore::RbacPolicy policy({
			.name = "name:RbacPoliciesTest.roles-add_invalid-policy",
		});

		const datastore::Role role({
			.name = "name:RbacPoliciesTest.roles-add_invalid-policy",
		});
		EXPECT_NO_THROW(role.store());

		auto rule = datastore::RbacPolicy::Rule({.roleId = role.id()});

		EXPECT_THROW(policy.addRule(rule), err::DatastoreInvalidRbacPolicyOrRole);
	}

	// Error: duplicate role
	{
		const datastore::RbacPolicy policy({
			.name = "name:RbacPoliciesTest.roles-add_duplicate-role",
		});
		EXPECT_NO_THROW(policy.store());

		const datastore::Role role({
			.name = "sub:RbacPoliciesTest.roles-add_duplicate-role",
		});
		EXPECT_NO_THROW(role.store());

		auto rule = datastore::RbacPolicy::Rule({.roleId = role.id()});

		EXPECT_NO_THROW(policy.addRule(rule));
		EXPECT_THROW(policy.addRule(rule), err::DatastoreDuplicateRbacPolicyRole);
	}

	// Success: list roles
	{
		const datastore::RbacPolicies policies({
			{{.name = "name:RbacPoliciesTest.roles-list[0]"}},
			{{.name = "name:RbacPoliciesTest.roles-list[1]"}},
		});

		for (const auto &policy : policies) {
			EXPECT_NO_THROW(policy.store());
		}

		const datastore::Roles roles({
			{{.name = "sub:RbacPoliciesTest.role-list[0]"}},
			{{.name = "sub:RbacPoliciesTest.role-list[1]"}},
		});

		for (const auto &role : roles) {
			EXPECT_NO_THROW(role.store());
		}

		const datastore::RbacPolicy::Rules rules({
			{.roleId = roles[0].id()},
			{.roleId = roles[1].id()},
		});

		EXPECT_NO_THROW(policies[0].addRule(rules[0]));
		EXPECT_NO_THROW(policies[0].addRule(rules[1]));

		{
			auto actualRoles = policies[0].rules();
			EXPECT_EQ(2, actualRoles.size());

			const datastore::RbacPolicy::Rules expected({
				{.roleId = roles[0].id()},
				{.roleId = roles[1].id()},
			});

			EXPECT_EQ(expected, actualRoles);
		}
		{
			auto actualRoles = policies[1].rules();
			EXPECT_EQ(0, actualRoles.size());
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