#include <gtest/gtest.h>

#include "err/errors.h"

#include "principals.h"
#include "records.h"
#include "testing.h"

class db_RecordsTest : public ::testing::Test {
protected:
	static void SetUpTestSuite() {
		db::testing::setup();

		// Clear data
		db::pg::exec("truncate table principals cascade;");
		db::pg::exec("truncate table records;");
	}

	void SetUp() {
		// Clear data before each test
		db::pg::exec("delete from records;");
	}

	static void TearDownTestSuite() { db::testing::teardown(); }
};

TEST_F(db_RecordsTest, discard) {
	db::Principal principal({
		.id = "id:db_RecordsTest.discard",
	});
	ASSERT_NO_THROW(principal.store());

	db::Record record({
		.principalId  = principal.id(),
		.resourceId   = "discard",
		.resourceType = "db_RecordsTest",
		.spaceId      = principal.spaceId(),
	});
	ASSERT_NO_THROW(record.store());

	bool result = false;
	ASSERT_NO_THROW(
		result = db::Record::discard(
			record.spaceId(), record.principalId(), record.resourceType(), record.resourceId()));
	EXPECT_TRUE(result);

	std::string_view qry = R"(
		select
			count(*)
		from records
		where
			space_id = $1::text
			and principal_id = $2::text
			and resource_type = $3::text
			and resource_id = $4::text
		;
	)";

	auto res = db::pg::exec(
		qry, record.spaceId(), record.principalId(), record.resourceType(), record.resourceId());
	ASSERT_EQ(1, res.size());

	auto count = res.at(0, 0).as<int>();
	EXPECT_EQ(0, count);
}

TEST_F(db_RecordsTest, list) {
	db::Principal principal({
		.id = "id:db_RecordsTest.list",
	});
	ASSERT_NO_THROW(principal.store());

	db::Record record({
		.principalId  = principal.id(),
		.resourceId   = "list",
		.resourceType = "db_RecordsTest",
		.spaceId      = principal.spaceId(),
	});
	ASSERT_NO_THROW(record.store());

	// Success: list by principal
	{
		db::Records results;
		ASSERT_NO_THROW(
			results = db::ListRecordsByPrincipal(
				principal.spaceId(), principal.id(), record.resourceType()));
		ASSERT_EQ(1, results.size());

		EXPECT_EQ(record, results[0]);
	}

	// Success: list by resource
	{
		db::Records results;
		ASSERT_NO_THROW(
			results = db::ListRecordsByResource(
				record.spaceId(), record.resourceType(), record.resourceId()));
		ASSERT_EQ(1, results.size());

		EXPECT_EQ(record, results[0]);
	}

	// Success: list with last id
	{
		db::Principals principals({
			{{.id = "id:db_RecordsTest.list-with_last_id[0]"}},
			{{.id = "id:db_RecordsTest.list-with_last_id[1]"}},
		});

		for (auto &p : principals) {
			ASSERT_NO_THROW(p.store());
		}

		db::Records records({
			{{
				.principalId  = principals[0].id(),
				.resourceId   = "list-with_last_id[0]",
				.resourceType = "db_RecordsTest",
				.spaceId      = principals[0].spaceId(),
			}},
			{{
				.principalId  = principals[1].id(),
				.resourceId   = "list-with_last_id[0]",
				.resourceType = "db_RecordsTest",
				.spaceId      = principals[1].spaceId(),
			}},
			{{
				.principalId  = principals[0].id(),
				.resourceId   = "list-with_last_id[1]",
				.resourceType = "db_RecordsTest",
				.spaceId      = principals[0].spaceId(),
			}},
			{{
				.principalId  = principals[1].id(),
				.resourceId   = "list-with_last_id[1]",
				.resourceType = "db_RecordsTest",
				.spaceId      = principals[1].spaceId(),
			}},
		});

		for (auto &r : records) {
			ASSERT_NO_THROW(r.store());
		}

		// by principal
		{
			db::Records results;
			ASSERT_NO_THROW({
				results = db::ListRecordsByPrincipal(
					principals[0].spaceId(),
					principals[0].id(),
					records[0].resourceType(),
					records[2].resourceId());
			});

			ASSERT_EQ(1, results.size());
			EXPECT_EQ(records[0], results[0]);
		}

		// by resource
		{
			db::Records results;
			ASSERT_NO_THROW({
				results = db::ListRecordsByResource(
					records[2].spaceId(),
					records[2].resourceType(),
					records[3].resourceId(),
					principals[1].id());
			});

			ASSERT_EQ(1, results.size());
			EXPECT_EQ(records[2], results[0]);
		}
	}
}

TEST_F(db_RecordsTest, lookup) {
	db::Principal principal({
		.id = "id:db_RecordsTest.lookup",
	});
	ASSERT_NO_THROW(principal.store());

	db::Record record({
		.principalId  = principal.id(),
		.resourceId   = "lookup",
		.resourceType = "db_RecordsTest",
		.spaceId      = principal.spaceId(),
	});
	ASSERT_NO_THROW(record.store());

	// Success: lookup record
	{
		auto result = db::Record::lookup(
			record.spaceId(), record.principalId(), record.resourceType(), record.resourceId());
		ASSERT_TRUE(result);

		EXPECT_EQ(record, result.value());
	}

	// Success: lookup non-existent record
	{
		auto result = db::Record::lookup(
			record.spaceId(), record.principalId(), record.resourceType(), "non-existent");
		EXPECT_EQ(std::nullopt, result);
	}
}

TEST_F(db_RecordsTest, rev) {
	db::Principal principal({
		.id = "id:db_RecordsTest.rev",
	});
	ASSERT_NO_THROW(principal.store());

	// Success: revision increment
	{
		db::Record record({
			.principalId  = principal.id(),
			.resourceId   = "rev",
			.resourceType = "db_RecordsTest",
			.spaceId      = principal.spaceId(),
		});

		ASSERT_NO_THROW(record.store());
		EXPECT_EQ(0, record.rev());

		ASSERT_NO_THROW(record.store());
		EXPECT_EQ(1, record.rev());
	}

	// Error: revision mismatch
	{
		db::Record record({
			.principalId  = principal.id(),
			.resourceId   = "rev-mismatch",
			.resourceType = "db_RecordsTest",
			.spaceId      = principal.spaceId(),
		});

		std::string_view qry = R"(
			insert into records (
				space_id,
				principal_id,
				resource_type,
				resource_id,
				_rev
			) values (
				$1::text,
				$2::text,
				$3::text,
				$4::text,
				$5::integer
			)
		)";

		ASSERT_NO_THROW(db::pg::exec(
			qry,
			record.spaceId(),
			record.principalId(),
			record.resourceType(),
			record.resourceId(),
			record.rev() + 1));

		EXPECT_THROW(record.store(), err::DbRevisionMismatch);
	}
}

TEST_F(db_RecordsTest, store) {
	db::Principal principal({
		.id = "id:db_RecordsTest.store",
	});
	ASSERT_NO_THROW(principal.store());

	// Success: persist data
	{
		db::Record record({
			.principalId  = principal.id(),
			.resourceId   = "store",
			.resourceType = "db_RecordsTest",
			.spaceId      = principal.spaceId(),
		});
		ASSERT_NO_THROW(record.store());

		std::string_view qry = R"(
			select
				attrs,
				_rev
			from records
			where
				space_id = $1::text
				and principal_id = $2::text
				and resource_type = $3::text
				and resource_id = $4::text
			;
		)";

		auto res = db::pg::exec(
			qry,
			record.spaceId(),
			record.principalId(),
			record.resourceType(),
			record.resourceId());
		ASSERT_EQ(1, res.size());

		auto [attrs, _rev] = res[0].as<db::Record::Data::attrs_t, int>();
		EXPECT_EQ(record.rev(), _rev);
		EXPECT_FALSE(attrs);
	}

	// Success:: persist data with optional attrs
	{
		db::Record record({
			.attrs        = R"(
				{
					"flag": true,
					"name": "First Last",
					"tags": [
						"test"
					]
				}
			)",
			.principalId  = principal.id(),
			.resourceId   = "store",
			.resourceType = "db_RecordsTest",
			.spaceId      = principal.spaceId(),
		});
		ASSERT_NO_THROW(record.store());

		std::string_view qry = R"(
			select
				attrs->'flag' as flag,
				attrs->>'name' as name,
				attrs->'tags' as tags
			from records
			where
				principal_id = $1::text
				and resource_type = $2::text
				and resource_id = $3::text
			;
		)";

		auto res =
			db::pg::exec(qry, record.principalId(), record.resourceType(), record.resourceId());
		ASSERT_EQ(1, res.size());

		auto [flag, name, tags] = res[0].as<bool, std::string, std::string>();
		EXPECT_EQ(true, flag);
		EXPECT_EQ("First Last", name);
		EXPECT_EQ(R"(["test"])", tags);
	}

	// Error: invalid `spaceId`
	{
		db::Record record({
			.principalId  = principal.id(),
			.resourceId   = "store-invalid_space_id",
			.resourceType = "db_RecordsTest",
			.spaceId      = "space_id:db_RecordsTest.store-invalid_space_id",
		});

		EXPECT_THROW(record.store(), err::DbRecordInvalidPrincipalId);
	}

	// Error: invalid `principalId`
	{
		db::Record record({
			.principalId  = "id:db_RecordsTest.store-invalid_principal_id",
			.resourceId   = "store-invalid_principal_id",
			.resourceType = "db_RecordsTest",
			.spaceId      = principal.spaceId(),
		});

		EXPECT_THROW(record.store(), err::DbRecordInvalidPrincipalId);
	}

	// Error: invalid `attrs`
	{
		db::Record record({
			.attrs        = R"("string")",
			.principalId  = principal.id(),
			.resourceId   = "store-invalid_attrs",
			.resourceType = "db_RecordsTest",
			.spaceId      = principal.spaceId(),
		});

		EXPECT_THROW(record.store(), err::DbRecordInvalidData);
	}
}
