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

TEST_F(db_TuplesTest, retrieve) {
	// Success: retrieve data
	{
		std::string_view qry = R"(
			insert into tuples (
				space_id,
				strand,
				l_entity_type,
				l_entity_id,
				relation,
				r_entity_type,
				r_entity_id,
				attrs,
				_id,
				_rev
			) values (
				$1::text,
				$2::text,
				$3::text,
				$4::text,
				$5::text,
				$6::text,
				$7::text,
				$8::jsonb,
				$9::text,
				$10::integer
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
