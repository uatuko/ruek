#include <gtest/gtest.h>

#include "err/errors.h"

#include "collections.h"
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
