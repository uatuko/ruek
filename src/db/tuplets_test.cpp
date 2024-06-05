#include <gtest/gtest.h>

#include "err/errors.h"

#include "testing.h"
#include "tuplets.h"

class db_TupletsTest : public testing::Test {
protected:
	static void SetUpTestSuite() {
		db::testing::setup();

		// Clear data
		db::pg::exec("truncate table tuples;");
	}

	void SetUp() {
		// Clear data from each test
		db::pg::exec("delete from tuples;");
	}

	static void TearDownTestSuite() { db::testing::teardown(); }
};

TEST_F(db_TupletsTest, list) {
	// Seed tuple to check other tests are only returning expected results
	db::Tuple tuple({
		.lEntityId   = "left",
		.lEntityType = "db_TupletsTest.list",
		.relation    = "relation",
		.rEntityId   = "right",
		.rEntityType = "db_TupletsTest.list",
		.strand      = "strand",
	});
	ASSERT_NO_THROW(tuple.store());

	// Success: list right
	{
		db::Tuple tuple({
			.lEntityId   = "left",
			.lEntityType = "db_TupletsTest.list-right",
			.relation    = "relation",
			.rEntityId   = "right",
			.rEntityType = "db_TupletsTest.list",
			.strand      = "strand",
		});
		ASSERT_NO_THROW(tuple.store());

		db::Tuplets results;
		ASSERT_NO_THROW(
			results =
				db::TupletsList(tuple.spaceId(), {{tuple.lEntityType(), tuple.lEntityId()}}, {}));

		ASSERT_EQ(1, results.size());

		const auto &actual = results.front();
		EXPECT_EQ(tuple.hashR(), actual.hash());
		EXPECT_EQ(tuple.id(), actual.id());
		EXPECT_EQ(tuple.relation(), actual.relation());
		EXPECT_FALSE(actual.strand());
	}

	// Success: list left
	{
		db::Tuple tuple({
			.lEntityId   = "left",
			.lEntityType = "db_TupletsTest.list",
			.relation    = "relation",
			.rEntityId   = "right",
			.rEntityType = "db_TupletsTest.list-left",
			.strand      = "strand",
		});
		ASSERT_NO_THROW(tuple.store());

		db::Tuplets results;
		ASSERT_NO_THROW(
			results =
				db::TupletsList(tuple.spaceId(), {}, {{tuple.rEntityType(), tuple.rEntityId()}}));

		ASSERT_EQ(1, results.size());

		const auto &actual = results.front();
		EXPECT_EQ(tuple.hashL(), actual.hash());
		EXPECT_EQ(tuple.id(), actual.id());
		EXPECT_EQ(tuple.relation(), actual.relation());
		EXPECT_EQ(tuple.strand(), actual.strand());
	}

	// Success: list with relation
	{
		db::Tuples tuples({
			{{
				.lEntityId   = "left",
				.lEntityType = "db_TupletsTest.list-with_relation",
				.relation    = "relation[0]",
				.rEntityId   = "right",
				.rEntityType = "db_TupletsTest.list-with_relation",
				.strand      = "strand",
			}},
			{{
				.lEntityId   = "left",
				.lEntityType = "db_TupletsTest.list-with_relation",
				.relation    = "relation[1]",
				.rEntityId   = "right",
				.rEntityType = "db_TupletsTest.list-with_relation",
				.strand      = "strand",
			}},
		});

		for (auto &t : tuples) {
			ASSERT_NO_THROW(t.store());
		}

		db::Tuplets results;
		ASSERT_NO_THROW(
			results = db::TupletsList(
				tuple.spaceId(),
				{},
				{{tuples[0].rEntityType(), tuples[0].rEntityId()}},
				tuples[0].relation()));

		ASSERT_EQ(1, results.size());

		const auto &actual = results.front();
		EXPECT_EQ(tuples[0].hashL(), actual.hash());
		EXPECT_EQ(tuples[0].id(), actual.id());
		EXPECT_EQ(tuples[0].relation(), actual.relation());
		EXPECT_EQ(tuples[0].strand(), actual.strand());
	}

	// Error: invalid args
	{
		EXPECT_THROW(
			db::TupletsList({}, db::Tuple::Entity(), db::Tuple::Entity()),
			err::DbTupletsInvalidListArgs);

		EXPECT_THROW(
			db::TupletsList({}, std::nullopt, std::nullopt), err::DbTupletsInvalidListArgs);
	}
}
