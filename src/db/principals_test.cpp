#include <gtest/gtest.h>

#include "err/errors.h"

#include "principals.h"
#include "testing.h"

class PrincipalsTest : public ::testing::Test {
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

TEST_F(PrincipalsTest, discard) {
	db::Principal principal({
		.id = "id:PrincipalsTest.discard",
	});

	ASSERT_NO_THROW(principal.store());
	ASSERT_NO_THROW(principal.discard());

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

TEST_F(PrincipalsTest, retrieve) {
	// Success: retrieve data
	{
		std::string_view qry = R"(
			insert into principals (
				_rev,
				id
			) values (
				$1::integer,
				$2::text
			);
		)";

		ASSERT_NO_THROW(db::pg::exec(qry, 1232, "id:PrincipalsTest.retrieve"));

		auto principal = db::RetrievePrincipal("id:PrincipalsTest.retrieve");
		EXPECT_EQ(1232, principal.rev());
		EXPECT_FALSE(principal.parentId());
		EXPECT_FALSE(principal.attrs());
	}

	// Success: retrieve optional attrs
	{
		std::string_view qry = R"(
			insert into principals (
				_rev,
				id,
				attrs
			) values (
				$1::integer,
				$2::text,
				$3::jsonb
			);
		)";

		ASSERT_NO_THROW(
			db::pg::exec(qry, 1237, "id:PrincipalsTest.retrieve-optional", R"({"foo": "bar"})"));

		auto principal = db::RetrievePrincipal("id:PrincipalsTest.retrieve-optional");
		EXPECT_EQ(1237, principal.rev());
		EXPECT_EQ(R"({"foo": "bar"})", principal.attrs());
	}
}

TEST_F(PrincipalsTest, rev) {
	// Success: revision increment
	{
		db::Principal principal({
			.id = "id:PrincipalsTest.rev",
		});

		ASSERT_NO_THROW(principal.store());
		EXPECT_EQ(0, principal.rev());

		ASSERT_NO_THROW(principal.store());
		EXPECT_EQ(1, principal.rev());
	}

	// Error: revision mismatch
	{
		db::Principal principal({
			.id = "id:PrincipalsTest.rev-mismatch",
		});

		std::string_view qry = R"(
			insert into principals (
				_rev,
				id
			) values (
				$1::integer,
				$2::text
			)
		)";

		ASSERT_NO_THROW(db::pg::exec(qry, principal.rev() + 1, principal.id()));

		EXPECT_THROW(principal.store(), err::DbRevisionMismatch);
	}
}

TEST_F(PrincipalsTest, store) {
	db::Principal principal({
		.id = "id:PrincipalsTest.store",
	});

	// Success: persist data
	{
		ASSERT_NO_THROW(principal.store());

		std::string_view qry = R"(
			select
				_rev,
				id,
				parent_id,
				attrs
			from principals
			where id = $1::text;
		)";

		auto res = db::pg::exec(qry, principal.id());
		EXPECT_EQ(1, res.size());

		auto [_rev, id, parentId, attrs] =
			res[0].as<int, std::string, db::Principal::Data::pid_t, db::Principal::Data::attrs_t>();
		EXPECT_EQ(principal.rev(), _rev);
		EXPECT_EQ(principal.id(), id);
		EXPECT_FALSE(parentId);
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
			.id    = "id:PrincipalsTest.store-optional",
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

	// Error: invalid `parentId`
	{
		db::Principal principal({
			.parentId = "id:PrincipalsTest.store-invalid-parentId",
		});

		EXPECT_THROW(principal.store(), err::DbInvalidPrincipalParentId);
	}

	// Error: invalid `attrs`
	{
		db::Principal principal({
			.attrs = R"("string")",
			.id    = "id:PrincipalsTest.store-invalid-attrs",
		});

		EXPECT_THROW(principal.store(), err::DbInvalidPrincipalData);
	}
}
