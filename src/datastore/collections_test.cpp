#include <gtest/gtest.h>

#include "err/errors.h"

#include "access-policies.h"
#include "collections.h"
#include "identities.h"
#include "testing.h"

class CollectionsTest : public ::testing::Test {
protected:
	static void SetUpTestSuite() {
		datastore::testing::setup();

		// Clear data
		datastore::pg::exec("truncate table collections cascade;");
	}

	void SetUp() {
		// Clear data before each test
		datastore::pg::exec("delete from collections cascade;");
	}

	static void TearDownTestSuite() { datastore::testing::teardown(); }
};

TEST_F(CollectionsTest, accessPolicies) {
	const datastore::Collection   collection({
		  .name = "name:CollectionsTest.accessPolicies",
    });
	const datastore::AccessPolicy access({
		.name = "access_policy_id:CollectionsTest.accessPolicies",
	});

	EXPECT_NO_THROW(collection.store());
	EXPECT_NO_THROW(access.store());
	EXPECT_NO_THROW(access.addCollection(collection.id()));

	EXPECT_EQ(1, collection.accessPolicies().size());
}

TEST_F(CollectionsTest, members) {
	// Success: add member
	{
		const datastore::Collection collection({
			.name = "name:CollectionsTest.members-add",
		});
		EXPECT_NO_THROW(collection.store());

		const datastore::Identity identity({
			.sub = "sub:CollectionsTest.members-add",
		});
		EXPECT_NO_THROW(identity.store());

		EXPECT_NO_THROW(collection.add(identity.id()));

		std::string_view qry = R"(
			select
				count(*) as n_members
			from collections_identities
			where
				collection_id = $1::text and
				identity_id = $2::text;
		)";

		auto res = datastore::pg::exec(qry, collection.id(), identity.id());
		EXPECT_EQ(1, res.at(0, 0).as<int>());

		// cleanup
		EXPECT_NO_THROW(identity.discard());
	}

	// Error: add invalid member
	{
		const datastore::Collection collection({
			.name = "name:CollectionsTest.members-add_invalid-member",
		});
		EXPECT_NO_THROW(collection.store());

		EXPECT_THROW(collection.add("invalid-member"), err::DatastoreInvalidCollectionOrMember);
	}

	// Error: add member to invalid collection
	{
		const datastore::Collection collection({
			.name = "name:CollectionsTest.members-add_invalid-collection",
		});

		const datastore::Identity identity({
			.sub = "sub:CollectionsTest.members-add_invalid-collection",
		});
		EXPECT_NO_THROW(identity.store());

		EXPECT_THROW(collection.add(identity.id()), err::DatastoreInvalidCollectionOrMember);
	}

	// Error: duplicate member
	{
		const datastore::Collection collection({
			.name = "name:CollectionsTest.members-add_duplicate-member",
		});
		EXPECT_NO_THROW(collection.store());

		const datastore::Identity identity({
			.sub = "sub:CollectionsTest.members-add_duplicate-member",
		});
		EXPECT_NO_THROW(identity.store());

		EXPECT_NO_THROW(collection.add(identity.id()));
		EXPECT_THROW(collection.add(identity.id()), err::DatastoreDuplicateCollectionMember);
	}

	// Success: list members
	{
		const datastore::Collections collections({
			{{.name = "name:CollectionsTest.members-list[0]"}},
			{{.name = "name:CollectionsTest.members-list[1]"}},
		});

		for (const auto &collection : collections) {
			EXPECT_NO_THROW(collection.store());
		}

		const datastore::Identities identities({
			{{.sub = "sub:CollectionsTest.members-list[0]"}},
			{{.sub = "sub:CollectionsTest.members-list[1]"}},
		});

		for (const auto &identity : identities) {
			EXPECT_NO_THROW(identity.store());
		}

		EXPECT_NO_THROW(collections[0].add(identities[0].id()));
		EXPECT_NO_THROW(collections[0].add(identities[1].id()));

		{
			auto members = collections[0].members();
			EXPECT_EQ(2, members.size());

			const datastore::Collection::members_t expected = {
				identities[0].id(), identities[1].id()};
			EXPECT_EQ(expected, members);
		}
		{
			auto members = collections[1].members();
			EXPECT_EQ(0, members.size());
		}

		// cleanup
		for (const auto &identity : identities) {
			EXPECT_NO_THROW(identity.discard());
		}
	}

	// Success: remove member
	{
		const datastore::Collection collection({
			.name = "name:CollectionsTest.members-remove",
		});
		EXPECT_NO_THROW(collection.store());

		const datastore::Identity identity({
			.sub = "sub:CollectionsTest.members-remove",
		});
		EXPECT_NO_THROW(identity.store());

		EXPECT_NO_THROW(collection.add(identity.id()));
		EXPECT_EQ(1, collection.members().size());

		EXPECT_NO_THROW(collection.remove(identity.id()));
		EXPECT_EQ(0, collection.members().size());

		// cleanup
		EXPECT_NO_THROW(identity.discard());
	}

	// Success: remove invalid member
	{
		const datastore::Collection collection({
			.name = "name:CollectionsTest.members-remove_invalid-member",
		});
		EXPECT_NO_THROW(collection.store());

		EXPECT_NO_THROW(collection.remove("invalid-member"));
	}
}

TEST_F(CollectionsTest, rbacPolicies) {
	const datastore::Collection collection({
		.name = "name:CollectionsTest.rbacPolicies",
	});
	const datastore::RbacPolicy rbac({
		.name = "rbac_policy_id:CollectionsTest.rbacPolicies",
	});

	EXPECT_NO_THROW(collection.store());
	EXPECT_NO_THROW(rbac.store());
	EXPECT_NO_THROW(rbac.addCollection(collection.id()));

	EXPECT_EQ(1, collection.rbacPolicies().size());
}

TEST_F(CollectionsTest, retrieve) {
	// Success: retrieve data
	{
		std::string_view qry = R"(
			insert into collections (
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
			datastore::pg::exec(
				qry, "_id:CollectionsTest.retrieve", 2248, "name:CollectionsTest.retrieve");
		} catch (const std::exception &e) {
			FAIL() << e.what();
		}

		auto collection = datastore::RetrieveCollection("_id:CollectionsTest.retrieve");
		EXPECT_EQ(2248, collection.rev());
		EXPECT_EQ("name:CollectionsTest.retrieve", collection.name());
	}

	// Success: retrieve members
	{
		const datastore::Collection collection({
			.name = "name:CollectionsTest.retrieve-members",
		});
		ASSERT_NO_THROW(collection.store());

		const datastore::Identity identity({
			.sub = "sub:CollectionsTest.retrieve-members",
		});
		ASSERT_NO_THROW(identity.store());

		// expect no entries before adding to collection
		{
			const auto members = datastore::RetrieveCollectionMembers(collection.id());
			EXPECT_EQ(0, members.size());
		}

		ASSERT_NO_THROW(collection.add(identity.id()));

		// expect single entry after adding one member
		{
			const auto members = datastore::RetrieveCollectionMembers(collection.id());
			EXPECT_EQ(1, members.size());
		}
	}

	// Error: not found
	{ EXPECT_THROW(datastore::RetrieveCollection("_id:dummy"), err::DatastoreCollectionNotFound); }
}

TEST_F(CollectionsTest, rev) {
	// Success: revision increment
	{
		const datastore::Collection collection({
			.name = "name:CollectionsTest.rev",
		});

		EXPECT_NO_THROW(collection.store());
		EXPECT_EQ(0, collection.rev());

		EXPECT_NO_THROW(collection.store());
		EXPECT_EQ(1, collection.rev());
	}

	// Error: revision mismatch
	{
		const datastore::Collection collection({
			.name = "name:CollectionsTest.rev-mismatch",
		});

		std::string_view qry = R"(
			insert into collections (
				_id,
				_rev,
				name
			) values (
				$1::text,
				$2::integer,
				$3::text
			)
		)";

		EXPECT_NO_THROW(
			datastore::pg::exec(qry, collection.id(), collection.rev() + 1, collection.name()));

		EXPECT_THROW(collection.store(), err::DatastoreRevisionMismatch);
	}
}

TEST_F(CollectionsTest, store) {
	const datastore::Collection collection({
		.name = "name:CollectionsTest.store",
	});

	EXPECT_NO_THROW(collection.store());

	std::string_view qry = R"(
		select
			_rev,
			name
		from collections
		where _id = $1::text;
	)";

	auto res = datastore::pg::exec(qry, collection.id());
	EXPECT_EQ(1, res.size());

	auto [_rev, name] = res[0].as<int, std::string>();
	EXPECT_EQ(collection.rev(), _rev);
	EXPECT_EQ(collection.name(), name);
}
