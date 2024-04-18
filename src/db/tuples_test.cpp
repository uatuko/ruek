#include <gtest/gtest.h>

#include "err/errors.h"

#include "common.h"
#include "testing.h"
#include "tuples.h"

class db_TuplesTest : public ::testing::Test {
protected:
	static void SetUpTestSuite() {
		db::testing::setup();

		// Clear data
		db::pg::exec("truncate table principals cascade;");
		db::pg::exec("truncate table tuples;");
	}

	void SetUp() {
		// Clear data from each test
		db::pg::exec("delete from tuples;");
	}

	static void TearDownTestSuite() { db::testing::teardown(); }
};

TEST_F(db_TuplesTest, discard) {
	db::Tuple tuple({
		.lEntityId   = "left",
		.lEntityType = "db_TuplesTest.discard",
		.relation    = "relation",
		.rEntityId   = "right",
		.rEntityType = "db_TuplesTest.discard",
	});
	ASSERT_NO_THROW(tuple.store());

	bool result = false;
	ASSERT_NO_THROW(result = db::Tuple::discard(tuple.id()));
	EXPECT_TRUE(result);

	std::string_view qry = R"(
		select
			count(*)
		from tuples
		where
			_id = $1::text;
	)";

	auto res = db::pg::exec(qry, tuple.id());
	ASSERT_EQ(1, res.size());

	auto count = res.at(0, 0).as<int>();
	EXPECT_EQ(0, count);

	// Idempotency
	ASSERT_NO_THROW(result = db::Tuple::discard(tuple.id()));
	EXPECT_FALSE(result);
}

TEST_F(db_TuplesTest, list) {
	// Seed tuple to check other tests are only returning expected results
	db::Tuple tuple({
		.lEntityId   = "left",
		.lEntityType = "db_TuplesTest.list",
		.relation    = "relation",
		.rEntityId   = "right",
		.rEntityType = "db_TuplesTest.list",
		.strand      = "strand",
	});
	ASSERT_NO_THROW(tuple.store());

	// Success: list right
	{
		db::Tuple tuple({
			.lEntityId   = "left",
			.lEntityType = "db_TuplesTest.list-right",
			.relation    = "relation",
			.rEntityId   = "right",
			.rEntityType = "db_TuplesTest.list",
			.strand      = "strand",
		});
		ASSERT_NO_THROW(tuple.store());

		db::Tuples results;
		ASSERT_NO_THROW(
			results =
				db::ListTuples(tuple.spaceId(), {{tuple.lEntityType(), tuple.lEntityId()}}, {}));
		ASSERT_EQ(1, results.size());
		EXPECT_EQ(tuple, results.front());
	}

	// Success: list left
	{
		db::Tuple tuple({
			.lEntityId   = "left",
			.lEntityType = "db_TuplesTest.list",
			.relation    = "relation",
			.rEntityId   = "right",
			.rEntityType = "db_TuplesTest.list-left",
			.strand      = "strand",
		});
		ASSERT_NO_THROW(tuple.store());

		db::Tuples results;
		ASSERT_NO_THROW(
			results =
				db::ListTuples(tuple.spaceId(), {}, {{tuple.rEntityType(), tuple.rEntityId()}}));
		ASSERT_EQ(1, results.size());
		EXPECT_EQ(tuple, results.front());
	}

	// Success: list with relation
	{
		db::Tuples tuples({
			{{
				.lEntityId   = "left",
				.lEntityType = "db_TuplesTest.list-with_relation",
				.relation    = "relation[0]",
				.rEntityId   = "right",
				.rEntityType = "db_TuplesTest.list-with_relation",
				.strand      = "strand",
			}},
			{{
				.lEntityId   = "left",
				.lEntityType = "db_TuplesTest.list-with_relation",
				.relation    = "relation[1]",
				.rEntityId   = "right",
				.rEntityType = "db_TuplesTest.list-with_relation",
				.strand      = "strand",
			}},
		});

		for (auto &t : tuples) {
			ASSERT_NO_THROW(t.store());
		}

		db::Tuples results;
		ASSERT_NO_THROW(
			results = db::ListTuples(
				tuples[0].spaceId(),
				{{tuples[0].lEntityType(), tuples[0].lEntityId()}},
				{},
				tuples[0].relation()));

		ASSERT_EQ(1, results.size());
		EXPECT_EQ(tuples[0], results.front());
	}

	// Success: list with last id
	{
		db::Tuples tuples({
			{{
				.lEntityId   = "left",
				.lEntityType = "db_TuplesTest.list-with_last_id",
				.relation    = "relation",
				.rEntityId   = "right-a",
				.rEntityType = "db_TuplesTest.list-with_last_id",
				.strand      = "strand",
			}},
			{{
				.lEntityId   = "left",
				.lEntityType = "db_TuplesTest.list-with_last_id",
				.relation    = "relation",
				.rEntityId   = "right-b",
				.rEntityType = "db_TuplesTest.list-with_last_id",
				.strand      = "strand",
			}},
		});

		for (auto &t : tuples) {
			ASSERT_NO_THROW(t.store());
		}

		db::Tuples results;
		ASSERT_NO_THROW(
			results = db::ListTuples(
				tuples[0].spaceId(),
				{{tuples[0].lEntityType(), tuples[0].lEntityId()}},
				{},
				{},
				tuples[1].rEntityId()));

		ASSERT_EQ(1, results.size());
		EXPECT_EQ(tuples[0], results.front());
	}

	// Success: list with last id and relation
	{
		db::Tuples tuples({
			{{
				.lEntityId   = "left-a",
				.lEntityType = "db_TuplesTest.list-with_last_id_and_relation",
				.relation    = "relation",
				.rEntityId   = "right",
				.rEntityType = "db_TuplesTest.list-with_last_id_and_relation",
				.strand      = "strand",
			}},
			{{
				.lEntityId   = "left-b",
				.lEntityType = "db_TuplesTest.list-with_last_id_and_relation",
				.relation    = "relation",
				.rEntityId   = "right",
				.rEntityType = "db_TuplesTest.list-with_last_id_and_relation",
				.strand      = "strand",
			}},
		});

		for (auto &t : tuples) {
			ASSERT_NO_THROW(t.store());
		}

		db::Tuples results;
		ASSERT_NO_THROW(
			results = db::ListTuples(
				tuples[0].spaceId(),
				{},
				{{tuples[0].rEntityType(), tuples[0].rEntityId()}},
				tuples[0].relation(),
				tuples[1].lEntityId()));

		ASSERT_EQ(1, results.size());
		EXPECT_EQ(tuples[0], results.front());
	}

	// Error: invalid args
	{
		EXPECT_THROW(
			db::ListTuples("", db::Tuple::Entity(), db::Tuple::Entity()),
			err::DbTuplesInvalidListArgs);

		EXPECT_THROW(db::ListTuples("", std::nullopt, std::nullopt), err::DbTuplesInvalidListArgs);
	}
}

TEST_F(db_TuplesTest, lookup) {
	// Success: lookup
	{
		db::Tuple tuple({
			.lEntityId   = "left",
			.lEntityType = "db_TuplesTest.lookup",
			.relation    = "relation",
			.rEntityId   = "right",
			.rEntityType = "db_TuplesTest.lookup",
			.strand      = "strand",
		});
		ASSERT_NO_THROW(tuple.store());

		auto results = db::LookupTuples(
			tuple.spaceId(),
			{tuple.lEntityType(), tuple.lEntityId()},
			tuple.relation(),
			{tuple.rEntityType(), tuple.rEntityId()},
			tuple.strand());

		ASSERT_EQ(1, results.size());
		EXPECT_EQ(tuple, results.front());
	}

	// Success: lookup without strand
	{
		db::Tuple tuple({
			.lEntityId   = "left",
			.lEntityType = "db_TuplesTest.lookup-without_strand",
			.relation    = "relation",
			.rEntityId   = "right",
			.rEntityType = "db_TuplesTest.lookup-without_strand",
			.strand      = "strand",
		});
		ASSERT_NO_THROW(tuple.store());

		db::Tuples results;
		ASSERT_NO_THROW(
			results = db::LookupTuples(
				tuple.spaceId(),
				{tuple.lEntityType(), tuple.lEntityId()},
				tuple.relation(),
				{tuple.rEntityType(), tuple.rEntityId()}));

		ASSERT_EQ(1, results.size());
		EXPECT_EQ(tuple, results.front());
	}

	// Success: lookup with last id
	{
		db::Tuples tuples({
			{{
				.lEntityId   = "left",
				.lEntityType = "db_TuplesTest.lookup-with_last_id",
				.relation    = "relation",
				.rEntityId   = "right",
				.rEntityType = "db_TuplesTest.lookup-with_last_id",
				.strand      = "strand[0]",
			}},
			{{
				.lEntityId   = "left",
				.lEntityType = "db_TuplesTest.lookup-with_last_id",
				.relation    = "relation",
				.rEntityId   = "right",
				.rEntityType = "db_TuplesTest.lookup-with_last_id",
				.strand      = "strand[1]",
			}},
		});

		for (auto &t : tuples) {
			ASSERT_NO_THROW(t.store());
		}

		db::Tuples results;
		ASSERT_NO_THROW(
			results = db::LookupTuples(
				tuples[1].spaceId(),
				{tuples[1].lEntityType(), tuples[1].lEntityId()},
				tuples[1].relation(),
				{tuples[1].rEntityType(), tuples[1].rEntityId()},
				std::nullopt,
				tuples[1].id()));
		ASSERT_EQ(1, results.size());

		EXPECT_EQ(tuples[0], results.front());
	}

	// Success: lookup with count
	{
		db::Tuples tuples({
			{{
				.lEntityId   = "left",
				.lEntityType = "db_TuplesTest.lookup-with_count",
				.relation    = "relation",
				.rEntityId   = "right",
				.rEntityType = "db_TuplesTest.lookup-with_count",
			}},
			{{
				.lEntityId   = "left",
				.lEntityType = "db_TuplesTest.lookup-with_count",
				.relation    = "relation",
				.rEntityId   = "right",
				.rEntityType = "db_TuplesTest.lookup-with_count",
				.strand      = "strand",
			}},
		});

		for (auto &t : tuples) {
			ASSERT_NO_THROW(t.store());
		}

		db::Tuples results;
		ASSERT_NO_THROW(
			results = db::LookupTuples(
				tuples[0].spaceId(),
				{tuples[0].lEntityType(), tuples[0].lEntityId()},
				tuples[0].relation(),
				{tuples[0].rEntityType(), tuples[0].rEntityId()},
				std::nullopt,
				"",
				1));
		ASSERT_EQ(1, results.size());

		EXPECT_EQ(tuples[1], results.front());
	}

	// Success: lookup non-existent
	{
		db::Tuple tuple({
			.lEntityId   = "left",
			.lEntityType = "db_TuplesTest.lookup-non_existent",
			.relation    = "relation",
			.rEntityId   = "right",
			.rEntityType = "db_TuplesTest.lookup-non_existent",
		});
		ASSERT_NO_THROW(tuple.store());

		db::Tuples results;
		ASSERT_NO_THROW(
			results = db::LookupTuples(
				tuple.spaceId(),
				{tuple.lEntityType(), tuple.lEntityId()},
				"non-existent",
				{tuple.rEntityType(), tuple.rEntityId()}));

		EXPECT_TRUE(results.empty());
	}
}

TEST_F(db_TuplesTest, retrieve) {
	// Success: retrieve data
	{
		std::string_view qry = R"(
			insert into tuples (
				space_id,
				strand,
				l_entity_type, l_entity_id,
				relation,
				r_entity_type, r_entity_id,
				attrs,
				_id, _rev
			) values (
				$1::text,
				$2::text,
				$3::text, $4::text,
				$5::text,
				$6::text, $7::text,
				$8::jsonb,
				$9::text, $10::integer
			);
		)";

		ASSERT_NO_THROW(db::pg::exec(
			qry,
			"",
			"",
			"db_TuplesTest.retrieve",
			"left",
			"relation",
			"db_TuplesTest.retrieve",
			"right",
			R"({"foo": "bar"})",
			"_id:db_TuplesTest.retrieve",
			1729));

		auto tuple = db::Tuple::retrieve("_id:db_TuplesTest.retrieve");
		EXPECT_FALSE(tuple.rid());
		EXPECT_EQ(1729, tuple.rev());

		EXPECT_EQ("db_TuplesTest.retrieve", tuple.lEntityType());
		EXPECT_EQ("left", tuple.lEntityId());

		EXPECT_EQ("relation", tuple.relation());

		EXPECT_EQ("db_TuplesTest.retrieve", tuple.rEntityType());
		EXPECT_EQ("right", tuple.rEntityId());

		EXPECT_EQ(R"({"foo": "bar"})", tuple.attrs());

		EXPECT_FALSE(tuple.lPrincipalId());
		EXPECT_FALSE(tuple.rPrincipalId());

		EXPECT_EQ("", tuple.spaceId());
		EXPECT_EQ("", tuple.strand());
	}

	// Error: not found
	{ EXPECT_THROW(db::Tuple::retrieve("dummy"), err::DbTupleNotFound); }
}

TEST_F(db_TuplesTest, rev) {
	// Success: revision increment
	{
		db::Tuple tuple({
			.lEntityId   = "left",
			.lEntityType = "db_TuplesTest.rev",
			.relation    = "relation",
			.rEntityId   = "right",
			.rEntityType = "db_TuplesTest.rev",
		});

		ASSERT_NO_THROW(tuple.store());
		EXPECT_EQ(0, tuple.rev());

		ASSERT_NO_THROW(tuple.store());
		EXPECT_EQ(1, tuple.rev());
	}

	// Error: revision mismatch
	{
		db::Tuple tuple({
			.lEntityId   = "left",
			.lEntityType = "db_TuplesTest.rev-mismatch",
			.relation    = "relation",
			.rEntityId   = "right",
			.rEntityType = "db_TuplesTest.rev-mismatch",
		});

		std::string_view qry = R"(
			insert into tuples as t (
				space_id,
				strand,
				l_entity_type, l_entity_id,
				relation,
				r_entity_type, r_entity_id,
				_id, _rev
			) values (
				$1::text,
				$2::text,
				$3::text, $4::text,
				$5::text,
				$6::text, $7::text,
				$8::text, $9::integer
			);
		)";

		ASSERT_NO_THROW(db::pg::exec(
			qry,
			tuple.spaceId(),
			tuple.strand(),
			tuple.lEntityType(),
			tuple.lEntityId(),
			tuple.relation(),
			tuple.rEntityType(),
			tuple.rEntityId(),
			tuple.id(),
			tuple.rev() + 1));

		EXPECT_THROW(tuple.store(), err::DbRevisionMismatch);
	}
}

TEST_F(db_TuplesTest, sanitise) {
	// Success: sanitise data
	{
		db::Tuple tuple({
			.lEntityId    = "lid:dummy",
			.lEntityType  = "ltype:dummy",
			.lPrincipalId = "left:db_TuplesTest.sanitise",
			.rEntityId    = "rid:dummy",
			.rEntityType  = "rtype:dummy",
			.rPrincipalId = "right:db_TuplesTest.sanitise",
		});

		EXPECT_EQ(db::common::principal_entity_v, tuple.lEntityType());
		EXPECT_EQ(tuple.lPrincipalId(), tuple.lEntityId());
		EXPECT_EQ(db::common::principal_entity_v, tuple.rEntityType());
		EXPECT_EQ(tuple.rPrincipalId(), tuple.rEntityId());
	}
}

TEST_F(db_TuplesTest, store) {
	// Success: persist data
	{
		db::Tuple tuple({
			.lEntityId   = "left",
			.lEntityType = "db_TuplesTest.store",
			.relation    = "relation",
			.rEntityId   = "right",
			.rEntityType = "db_TuplesTest.store",
		});
		ASSERT_NO_THROW(tuple.store());

		std::string_view qry = R"(
			select
				space_id,
				strand,
				l_entity_type, l_entity_id,
				relation,
				r_entity_type, r_entity_id,
				attrs,
				l_principal_id, r_principal_id,
				_id, _rid, _rev
			from tuples
			where _id = $1::text;
		)";

		auto res = db::pg::exec(qry, tuple.id());
		ASSERT_EQ(1, res.size());

		auto
			[spaceId,
			 strand,
			 lEntityType,
			 lEntityId,
			 relation,
			 rEntityType,
			 rEntityId,
			 attrs,
			 lPrincipalId,
			 rPrincipalId,
			 _id,
			 _rid,
			 _rev] =
				res[0]
					.as<std::string,
						std::string,
						std::string,
						std::string,
						std::string,
						std::string,
						std::string,
						db::Tuple::Data::attrs_t,
						db::Tuple::Data::pid_t,
						db::Tuple::Data::pid_t,
						std::string,
						db::Tuple::rid_t,
						int>();

		EXPECT_EQ(tuple.spaceId(), spaceId);
		EXPECT_EQ(tuple.strand(), strand);
		EXPECT_EQ(tuple.lEntityType(), lEntityType);
		EXPECT_EQ(tuple.lEntityId(), lEntityId);
		EXPECT_EQ(tuple.relation(), relation);
		EXPECT_EQ(tuple.rEntityType(), rEntityType);
		EXPECT_EQ(tuple.rEntityId(), rEntityId);
		EXPECT_EQ(tuple.attrs(), attrs);
		EXPECT_EQ(tuple.lPrincipalId(), lPrincipalId);
		EXPECT_EQ(tuple.rPrincipalId(), rPrincipalId);
		EXPECT_EQ(tuple.id(), _id);
		EXPECT_EQ(tuple.rev(), _rev);
		EXPECT_EQ(tuple.rid(), _rid);
	}

	// Error: invalid `attrs`
	{
		db::Tuple tuple({
			.attrs       = R"("string")",
			.lEntityId   = "left",
			.lEntityType = "db_TuplesTest.store-invalid_attrs",
			.relation    = "relation",
			.rEntityId   = "right",
			.rEntityType = "db_TuplesTest.store-invalid_attrs",
		});

		EXPECT_THROW(tuple.store(), err::DbTupleInvalidData);
	}

	// Error: invalid `l_principal_id`
	{
		db::Tuple tuple({
			.lPrincipalId = "dummy",
			.relation     = "relation",
			.rEntityId    = "right",
			.rEntityType  = "db_TuplesTest.store-invalid_l_principal_id",
		});

		EXPECT_THROW(tuple.store(), err::DbTupleInvalidKey);
	}
}
