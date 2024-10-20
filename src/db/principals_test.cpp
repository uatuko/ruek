#include <gtest/gtest.h>

#include "err/errors.h"

#include "principals.h"
#include "testing.h"

class db_PrincipalsTest : public ::testing::Test {
protected:
	static void SetUpTestSuite() {
		db::testing::setup();

		// Clear data
		db::pg::exec("truncate table principals cascade;");
	}

	void SetUp() {
		// Clear data before each test
		db::pg::exec("delete from principals cascade;");
	}

	static void TearDownTestSuite() { db::testing::teardown(); }
};

TEST_F(db_PrincipalsTest, discard) {
	db::Principal principal({
		.id = "id:db_PrincipalsTest.discard",
	});
	ASSERT_NO_THROW(principal.store());

	bool result = false;
	ASSERT_NO_THROW(result = db::Principal::discard("", principal.id()));
	EXPECT_TRUE(result);

	std::string_view qry = R"(
		select
			count(*)
		from principals
		where id = $1::text;
	)";

	auto res = db::pg::exec(qry, principal.id());
	ASSERT_EQ(1, res.size());

	auto count = res.at(0, 0).as<int>();
	EXPECT_EQ(0, count);
}

TEST_F(db_PrincipalsTest, list) {
	// Success: list
	{
		db::Principal principal({
			.id = "id:db_PrincipalsTest.list",
		});
		ASSERT_NO_THROW(principal.store());

		db::Principals results;
		ASSERT_NO_THROW(results = db::ListPrincipals(""));
		ASSERT_EQ(1, results.size());

		EXPECT_EQ(principal, results[0]);
	}

	// Success: list with segment
	{
		db::Principal principal({
			.id      = "id:db_PrincipalsTest.list-with_segment",
			.segment = "segment:db_PrincipalsTest.list-with_segment",
		});
		ASSERT_NO_THROW(principal.store());

		db::Principals results;
		ASSERT_NO_THROW(results = db::ListPrincipals("", principal.segment()));
		ASSERT_EQ(1, results.size());

		EXPECT_EQ(principal, results[0]);
	}

	// Success: list with space id
	{
		db::Principal principal({
			.id      = "id:db_PrincipalsTest.list-with_space_id",
			.spaceId = "space_id:db_PrincipalsTest.list-with_space_id",
		});
		ASSERT_NO_THROW(principal.store());

		db::Principals results;
		ASSERT_NO_THROW(results = db::ListPrincipals(principal.spaceId()));
		ASSERT_EQ(1, results.size());

		EXPECT_EQ(principal, results[0]);
	}

	// Success: list with last id
	// WARNING: this test can break depending on other tests
	{
		db::Principals principals({
			{{.id = "a_id:db_PrincipalsTest.list-with_last_id[0]"}},
			{{.id = "a_id:db_PrincipalsTest.list-with_last_id[1]"}},
		});

		for (auto &p : principals) {
			ASSERT_NO_THROW(p.store());
		}

		db::Principals results;
		ASSERT_NO_THROW(results = db::ListPrincipals("", std::nullopt, principals[1].id()));
		ASSERT_EQ(1, results.size());

		EXPECT_EQ(principals[0], results[0]);
	}

	// Success: list with segment and last id
	{
		db::Principals principals({
			{{
				.id      = "id:db_PrincipalsTest.list-with_segment_and_last_id[0]",
				.segment = "segment:db_PrincipalsTest.list-with_segment_and_last_id",
			}},
			{{
				.id      = "id:db_PrincipalsTest.list-with_segment_and_last_id[1]",
				.segment = "segment:db_PrincipalsTest.list-with_segment_and_last_id",
			}},
		});

		for (auto &p : principals) {
			ASSERT_NO_THROW(p.store());
		}

		db::Principals results;
		ASSERT_NO_THROW(
			results = db::ListPrincipals("", principals[0].segment(), principals[1].id()));
		ASSERT_EQ(1, results.size());

		EXPECT_EQ(principals[0], results[0]);
	}

	// Success: list with count (and segment)
	{
		db::Principals principals({
			{{
				.id      = "id:db_PrincipalsTest.list-with_count[0]",
				.segment = "segment:db_PrincipalsTest.list-with_count",
			}},
			{{
				.id      = "id:db_PrincipalsTest.list-with_count[1]",
				.segment = "segment:db_PrincipalsTest.list-with_count",
			}},
		});

		for (auto &p : principals) {
			ASSERT_NO_THROW(p.store());
		}

		db::Principals results;
		ASSERT_NO_THROW(results = db::ListPrincipals("", principals[0].segment(), "", 1));
		ASSERT_EQ(1, results.size());

		EXPECT_EQ(principals[1], results[0]);
	}
}

TEST_F(db_PrincipalsTest, retrieve) {
	// Success: retrieve data
	{
		std::string_view qry = R"(
			insert into principals (
				space_id,
				id,
				_rev
			) values (
				$1::text,
				$2::text,
				$3::integer
			);
		)";

		ASSERT_NO_THROW(db::pg::exec(qry, "", "id:db_PrincipalsTest.retrieve", 1232));

		auto principal = db::Principal::retrieve("", "id:db_PrincipalsTest.retrieve");
		EXPECT_EQ(1232, principal.rev());
		EXPECT_EQ("", principal.spaceId());
		EXPECT_FALSE(principal.segment());
		EXPECT_FALSE(principal.attrs());
	}

	// Success: retrieve optional attrs
	{
		std::string_view qry = R"(
			insert into principals (
				space_id,
				id,
				attrs,
				_rev
			) values (
				$1::text,
				$2::text,
				$3::jsonb,
				$4::integer
			);
		)";

		ASSERT_NO_THROW(db::pg::exec(
			qry, "", "id:db_PrincipalsTest.retrieve-optional", R"({"foo": "bar"})", 1237));

		auto principal = db::Principal::retrieve("", "id:db_PrincipalsTest.retrieve-optional");
		EXPECT_EQ(1237, principal.rev());
		EXPECT_EQ(R"({"foo": "bar"})", principal.attrs());
	}

	// Error: not found (space id mismatch)
	{
		std::string_view qry = R"(
			insert into principals (
				space_id,
				id,
				_rev
			) values (
				$1::text,
				$2::text,
				$3::integer
			);
		)";

		ASSERT_NO_THROW(db::pg::exec(
			qry,
			"space_id:db_PrincipalsTest.retrieve-space_id_mismatch",
			"id:db_PrincipalsTest.retrieve-space_id_mismatch",
			933));

		EXPECT_THROW(
			db::Principal::retrieve("", "id:db_PrincipalsTest.retrieve-space_id_mismatch"),
			err::DbPrincipalNotFound);
	}
}

TEST_F(db_PrincipalsTest, rev) {
	// Success: revision
	{
		const db::Principal::Data data{
			.id = "id:db_PrincipalsTest.rev",
		};

		db::Principal pl(data), pr(data);
		EXPECT_FALSE(pl == pr);
		EXPECT_FALSE(pl.rev() == pr.rev());
		EXPECT_EQ(pl.id(), pr.id());
	}

	// Success: revision increment
	{
		db::Principal principal({
			.id = "id:db_PrincipalsTest.rev",
		});

		ASSERT_NO_THROW(principal.store());

		auto expected = principal.rev() + 1;
		ASSERT_NO_THROW(principal.store());
		EXPECT_EQ(expected, principal.rev());
	}

	// Error: revision mismatch
	{
		db::Principal principal({
			.id = "id:db_PrincipalsTest.rev-mismatch",
		});

		std::string_view qry = R"(
			insert into principals (
				space_id,
				id,
				_rev
			) values (
				$1::text,
				$2::text,
				$3::integer
			)
		)";

		ASSERT_NO_THROW(
			db::pg::exec(qry, principal.spaceId(), principal.id(), principal.rev() + 1));

		EXPECT_THROW(principal.store(), err::DbRevisionMismatch);
	}
}

TEST_F(db_PrincipalsTest, store) {
	db::Principal principal({
		.id = "id:db_PrincipalsTest.store",
	});

	// Success: persist data
	{
		ASSERT_NO_THROW(principal.store());

		std::string_view qry = R"(
			select
				space_id,
				id,
				segment,
				attrs,
				_rev
			from principals
			where id = $1::text;
		)";

		auto res = db::pg::exec(qry, principal.id());
		ASSERT_EQ(1, res.size());

		auto [spaceId, id, segment, attrs, _rev] =
			res[0]
				.as<std::string,
					std::string,
					db::Principal::Data::segment_t,
					db::Principal::Data::attrs_t,
					int>();

		EXPECT_EQ(principal.spaceId(), spaceId);
		EXPECT_EQ(principal.id(), id);
		EXPECT_EQ(principal.rev(), _rev);
		EXPECT_FALSE(segment);
		EXPECT_FALSE(attrs);
	}

	// Success: persist data with optional attrs
	{
		db::Principal principal({
			.attrs = R"(
				{
					"flag": true,
					"name": "First Last",
					"tags": [
						"test"
					]
				}
			)",
			.id    = "id:db_PrincipalsTest.store-optional",
		});
		ASSERT_NO_THROW(principal.store());

		std::string_view qry = R"(
			select
				attrs->'flag' as flag,
				attrs->>'name' as name,
				attrs->'tags' as tags
			from principals
			where id = $1::text;
		)";

		auto res = db::pg::exec(qry, principal.id());
		ASSERT_EQ(1, res.size());

		auto [flag, name, tags] = res[0].as<bool, std::string, std::string>();
		EXPECT_EQ(true, flag);
		EXPECT_EQ("First Last", name);
		EXPECT_EQ(R"(["test"])", tags);
	}

	// Error: invalid `segment`
	{
		db::Principal principal({
			.segment = "",
		});

		EXPECT_THROW(principal.store(), err::DbPrincipalInvalidData);
	}

	// Error: invalid `attrs`
	{
		db::Principal principal({
			.attrs = R"("string")",
			.id    = "id:db_PrincipalsTest.store-invalid_attrs",
		});

		EXPECT_THROW(principal.store(), err::DbPrincipalInvalidData);
	}
}
