#include <gtest/gtest.h>

#include "err/errors.h"

#include "collections.h"
#include "identities.h"
#include "testing.h"

class IdentitiesTest : public ::testing::Test {
protected:
	static void SetUpTestSuite() {
		datastore::testing::setup();

		// Clear data
		datastore::pg::exec("truncate table identities cascade;");
	}

	void SetUp() {
		// Clear data before each test
		datastore::pg::exec("delete from identities cascade;");
	}

	static void TearDownTestSuite() { datastore::testing::teardown(); }
};

TEST_F(IdentitiesTest, discard) {
	const datastore::Identity identity({
		.sub = "sub:IdentitiesTest.discard",
	});

	ASSERT_NO_THROW(identity.store());
	ASSERT_NO_THROW(identity.discard());

	std::string_view qry = R"(
		select
			count(*)
		from identities
		where _id = $1::text;
	)";

	auto res = datastore::pg::exec(qry, identity.id());
	ASSERT_EQ(1, res.size());

	auto count = res.at(0, 0).as<int>();
	EXPECT_EQ(0, count);
}

TEST_F(IdentitiesTest, listInCollection) {
	// Success:
	{
		const datastore::Identity identity({
			.sub = "sub:IdentitiesTest.listInCollection",
		});
		ASSERT_NO_THROW(identity.store());

		const datastore::Collection collection({
			.name = "collection:IdentitiesTest.listInCollection",
		});
		ASSERT_NO_THROW(collection.store());

		// expect no entries before adding to collection
		auto identities = datastore::ListIdentitiesInCollection(collection.id());
		EXPECT_EQ(0, identities.size());

		ASSERT_NO_THROW(collection.add(identity.id()));

		// expect single entry
		identities = datastore::ListIdentitiesInCollection(collection.id());
		EXPECT_EQ(1, identities.size());
	}
}

TEST_F(IdentitiesTest, lookup) {
	// Success:
	{
		const datastore::Identity identity({
			.sub = "sub:IdentitiesTest.lookup",
		});

		// expect no entries before adding to collection
		auto identities = datastore::LookupIdentities(identity.sub());
		EXPECT_EQ(0, identities.size());

		ASSERT_NO_THROW(identity.store());

		// expect single entry
		identities = datastore::LookupIdentities(identity.sub());
		EXPECT_EQ(1, identities.size());
	}
}

TEST_F(IdentitiesTest, retrieve) {
	// Success: retrieve data
	{
		std::string_view qry = R"(
			insert into identities (
				_id,
				_rev,
				sub
			) values (
				$1::text,
				$2::integer,
				$3::text
			);
		)";

		try {
			datastore::pg::exec(
				qry, "_id:IdentitiesTest.retrieve", 2308, "sub:IdentitiesTest.retrieve");
		} catch (const std::exception &e) {
			FAIL() << e.what();
		}

		auto identity = datastore::RetrieveIdentity("_id:IdentitiesTest.retrieve");
		EXPECT_EQ(2308, identity.rev());
		EXPECT_EQ("sub:IdentitiesTest.retrieve", identity.sub());
		EXPECT_FALSE(identity.attrs());
	}

	// Success: retrieve optional attrs
	{
		std::string_view qry = R"(
			insert into identities (
				_id,
				_rev,
				sub,
				attrs
			) values (
				$1::text,
				$2::integer,
				$3::text,
				$4::jsonb
			);
		)";

		try {
			datastore::pg::exec(
				qry,
				"_id:IdentitiesTest.retrieve-optional",
				1418,
				"sub:IdentitiesTest.retrieve-optional",
				R"({"foo": "bar"})");
		} catch (const std::exception &e) {
			FAIL() << e.what();
		}

		auto identity = datastore::RetrieveIdentity("_id:IdentitiesTest.retrieve-optional");
		EXPECT_EQ(1418, identity.rev());
		EXPECT_EQ(R"({"foo": "bar"})", identity.attrs());
	}

	// Error: not found
	{ EXPECT_THROW(datastore::RetrieveIdentity("_id:dummy"), err::DatastoreIdentityNotFound); }
}

TEST_F(IdentitiesTest, rev) {
	// Success: revision increment
	{
		const datastore::Identity identity({
			.sub = "sub:IdentitiesTest.rev",
		});

		ASSERT_NO_THROW(identity.store());
		EXPECT_EQ(0, identity.rev());

		ASSERT_NO_THROW(identity.store());
		EXPECT_EQ(1, identity.rev());
	}

	// Error: revision mismatch
	{
		const datastore::Identity identity({
			.sub = "sub:IdentitiesTest.rev-mismatch",
		});

		std::string_view qry = R"(
			insert into identities (
				_id,
				_rev,
				sub
			) values (
				$1::text,
				$2::integer,
				$3::text
			)
		)";

		ASSERT_NO_THROW(
			datastore::pg::exec(qry, identity.id(), identity.rev() + 1, identity.sub()));

		EXPECT_THROW(identity.store(), err::DatastoreRevisionMismatch);
	}
}

TEST_F(IdentitiesTest, store) {
	const datastore::Identity identity({
		.sub = "sub:IdentitiesTest.store",
	});

	// Success: persist data
	{
		ASSERT_NO_THROW(identity.store());

		std::string_view qry = R"(
			select
				_id,
				_rev,
				sub,
				attrs
			from identities
			where _id = $1::text;
		)";

		auto res = datastore::pg::exec(qry, identity.id());
		EXPECT_EQ(1, res.size());

		auto [_id, _rev, sub, attrs] =
			res[0].as<std::string, int, std::string, datastore::Identity::Data::attrs_t>();
		EXPECT_EQ(identity.id(), _id);
		EXPECT_EQ(identity.rev(), _rev);
		EXPECT_EQ(identity.sub(), sub);
		EXPECT_FALSE(attrs);
	}

	// Success: persist data with optional attrs
	{
		const datastore::Identity identity({
			.attrs = R"(
				{
					"flag": true,
					"name": "First Last",
					"tags": [
						"test"
					]
				}
			)",
			.sub   = "sub:IdentitiesTest.store-optional",
		});
		ASSERT_NO_THROW(identity.store());

		std::string_view qry = R"(
			select
				attrs->'flag' as flag,
				attrs->>'name' as name,
				attrs->'tags' as tags
			from identities
			where _id = $1::text;
		)";

		auto res = datastore::pg::exec(qry, identity.id());
		ASSERT_EQ(1, res.size());

		auto [flag, name, tags] = res[0].as<bool, std::string, std::string>();
		EXPECT_EQ(true, flag);
		EXPECT_EQ("First Last", name);
		EXPECT_EQ(R"(["test"])", tags);
	}

	// Error: duplicate `sub`
	{
		const datastore::Identity duplicate({
			.sub = identity.sub(),
		});

		EXPECT_THROW(duplicate.store(), err::DatastoreDuplicateIdentity);
	}

	// Error: invalid `attrs`
	{
		const datastore::Identity identity({
			.attrs = R"("string")",
			.sub   = "sub:IdentitiesTest.store-invalid-attrs",
		});

		EXPECT_THROW(identity.store(), err::DatastoreInvalidIdentityData);
	}
}
