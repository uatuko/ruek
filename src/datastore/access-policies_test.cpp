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

TEST_F(AccessPoliciesTest, identities) {
	// Success: retrieve identities
	{
		// Create background noise
		const datastore::Identities randomIdentities({
			datastore::Identity::Data{
				.sub = "sub:AccessPoliciesTest.retrieveIdentities.randomIdentities[0]"},
			datastore::Identity::Data{
				.sub = "sub:AccessPoliciesTest.retrieveIdentities.randomIdentities[1]"},
		});

		for (const auto &idn : randomIdentities) {
			ASSERT_NO_THROW(idn.store());
		}

		const datastore::Collection randomCollection({
			.name = "name:AccessPoliciesTest.retrieveIdentities.randomCollection",
		});
		ASSERT_NO_THROW(randomCollection.store());
		ASSERT_NO_THROW(randomCollection.add(randomIdentities[0].id()));

		const datastore::AccessPolicy randomPolicy({
			.name = "name:AccessPoliciesTest.retrieveIdentities.randomPolicy",
		});
		ASSERT_NO_THROW(randomPolicy.store());
		ASSERT_NO_THROW(randomPolicy.addCollection(randomCollection.id()));
		ASSERT_NO_THROW(randomPolicy.addIdentity(randomIdentities[1].id()));

		// Setup real tests
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

		ASSERT_NO_THROW(policy.addCollection(collection.id()));
		ASSERT_NO_THROW(policy.addIdentity(identities[1].id()));

		// without expand should only return direct identities
		{
			const auto results = policy.identities();
			EXPECT_EQ(1, results.size());
			EXPECT_TRUE(results.contains(identities[1].id()));
		}

		// with expand should return direct and indirect identities
		{
			const auto results = policy.identities(true);
			EXPECT_EQ(2, results.size());

			for (const auto &idn : identities) {
				EXPECT_TRUE(results.contains(idn.id()));
			}
		}
	}
}

TEST_F(AccessPoliciesTest, retrieve) {
	// Success: retrieve data
	{
		std::string_view qry = R"(
			insert into "access-policies" (
				_id,
				_rev,
				rules
			) values (
				$1::text,
				$2::integer,
				$3::jsonb
			);
		)";

		try {
			datastore::pg::exec(
				qry,
				"_id:AccessPoliciesTest.retrieve",
				1328,
				R"([{"attrs": "attrs:AccessPoliciesTest.retrieve", "resource":"resource:AccessPoliciesTest.retrieve"}])");
		} catch (const std::exception &e) {
			FAIL() << e.what();
		}

		auto policy = datastore::RetrieveAccessPolicy("_id:AccessPoliciesTest.retrieve");
		EXPECT_EQ(1328, policy.rev());
		EXPECT_FALSE(policy.name());

		const auto &rules = policy.rules();
		ASSERT_EQ(1, rules.size());

		const auto rule = rules.cbegin();
		EXPECT_EQ("attrs:AccessPoliciesTest.retrieve", rule->attrs);
		EXPECT_EQ("resource:AccessPoliciesTest.retrieve", rule->resource);
	}

	// Success: retrieve by collection
	{
		const datastore::Collection collection({
			.name = "name:AccessPoliciesTest.retrieve-by_collection",
		});
		ASSERT_NO_THROW(collection.store());

		const datastore::AccessPolicy policy({
			.name = "name:AccessPoliciesTest.retrieve-by_collectio",
		});
		ASSERT_NO_THROW(policy.store());
		ASSERT_NO_THROW(policy.addCollection(collection.id()));

		const auto results = datastore::RetrieveAccessPoliciesByCollection(collection.id());
		ASSERT_EQ(1, results.size());
		EXPECT_EQ(policy.id(), results.front().id());
	}
}

TEST_F(AccessPoliciesTest, rev) {
	// Success: revision increment
	{
		const datastore::AccessPolicy policy({
			.name = "name:AccessPoliciesTest.rev",
		});
		ASSERT_NO_THROW(policy.store());
		EXPECT_EQ(0, policy.rev());

		EXPECT_NO_THROW(policy.store());
		EXPECT_EQ(1, policy.rev());
	}

	// Error: revision mismatch
	{
		const datastore::AccessPolicy policy({
			.name = "name:AccessPoliciesTest.rev-mismatch",
		});

		std::string_view qry = R"(
			insert into "access-policies" (
				_id,
				_rev,
				name
			) values (
				$1::text,
				$2::integer,
				$3::text
			)
		)";

		ASSERT_NO_THROW(datastore::pg::exec(qry, policy.id(), policy.rev() + 1, policy.name()));
		EXPECT_THROW(policy.store(), err::DatastoreRevisionMismatch);
	}
}

TEST_F(AccessPoliciesTest, store) {
	const datastore::AccessPolicy policy({
		.name = "name:AccessPoliciesTest.store",
		.rules =
			{
				{
					.attrs    = R"({"key":"value"})",
					.resource = "resource/AccessPoliciesTest.store",
				},
			},
	});
	ASSERT_NO_THROW(policy.store());

	std::string_view qry = R"(
		select
			_rev,
			name,
			rules
		from "access-policies"
		where _id = $1::text;
	)";

	auto res = datastore::pg::exec(qry, policy.id());
	ASSERT_EQ(1, res.size());

	auto [_rev, name, rules] = res[0].as<int, std::string, std::string>();
	EXPECT_EQ(policy.rev(), _rev);
	EXPECT_EQ(policy.name(), name);
	EXPECT_EQ(
		R"([{"attrs": "{\"key\":\"value\"}", "resource": "resource/AccessPoliciesTest.store"}])",
		rules);
}
