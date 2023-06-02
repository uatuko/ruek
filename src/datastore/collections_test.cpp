#include <gtest/gtest.h>

#include "err/errors.h"

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

TEST_F(CollectionsTest, members) {
	// Success: add member
	{
		const auto collection = datastore::Collection({
			.name = "name:CollectionsTest.members-add",
		});
		EXPECT_NO_THROW(collection.store());

		const auto identity = datastore::Identity({
			.sub = "sub:CollectionsTest.members-add",
		});
		EXPECT_NO_THROW(identity.store());

		EXPECT_NO_THROW(collection.add(identity.id()));

		std::string_view qry = R"(
			select
				count(*) as n_members
			from collection_members
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
		const auto collection = datastore::Collection({
			.name = "name:CollectionsTest.members-add_invalid-member",
		});
		EXPECT_NO_THROW(collection.store());

		EXPECT_THROW(collection.add("invalid-member"), err::DatastoreInvalidCollectionOrMember);
	}

	// Error: add member to invalid collection
	{
		const auto collection = datastore::Collection({
			.name = "name:CollectionsTest.members-add_invalid-collection",
		});

		const auto identity = datastore::Identity({
			.sub = "sub:CollectionsTest.members-add_invalid-collection",
		});
		EXPECT_NO_THROW(identity.store());

		EXPECT_THROW(collection.add(identity.id()), err::DatastoreInvalidCollectionOrMember);
	}

	// Error: duplicate member
	{
		const auto collection = datastore::Collection({
			.name = "name:CollectionsTest.members-add_duplicate-member",
		});
		EXPECT_NO_THROW(collection.store());

		const auto identity = datastore::Identity({
			.sub = "sub:CollectionsTest.members-add_duplicate-member",
		});
		EXPECT_NO_THROW(identity.store());

		EXPECT_NO_THROW(collection.add(identity.id()));
		EXPECT_THROW(collection.add(identity.id()), err::DatastoreDuplicateCollectionMember);
	}

	// Success: list members
	{
		const auto collections = datastore::Collections({
			{{.name = "name:CollectionsTest.members-list[0]"}},
			{{.name = "name:CollectionsTest.members-list[1]"}},
		});

		for (const auto &collection : collections) {
			EXPECT_NO_THROW(collection.store());
		}

		const auto identities = datastore::Identities({
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
		const auto collection = datastore::Collection({
			.name = "name:CollectionsTest.members-remove",
		});
		EXPECT_NO_THROW(collection.store());

		const auto identity = datastore::Identity({
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
		const auto collection = datastore::Collection({
			.name = "name:CollectionsTest.members-remove_invalid-member",
		});
		EXPECT_NO_THROW(collection.store());

		EXPECT_NO_THROW(collection.remove("invalid-member"));
	}
}

TEST_F(CollectionsTest, rev) {
	// Success: revision increment
	{
		const auto collection = datastore::Collection({
			.name = "name:CollectionsTest.rev",
		});

		EXPECT_NO_THROW(collection.store());
		EXPECT_EQ(0, collection.rev());

		EXPECT_NO_THROW(collection.store());
		EXPECT_EQ(1, collection.rev());
	}

	// Error: revision mismatch
	{
		const auto collection = datastore::Collection({
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
	const auto collection = datastore::Collection({
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
