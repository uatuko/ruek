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
	});

	ASSERT_NO_THROW(record.store());
	ASSERT_NO_THROW(record.discard());

	std::string_view qry = R"(
		select
			count(*)
		from records
		where
			principal_id = $1::text
			and resource_type = $2::text
			and resource_id = $3::text
		;
	)";

	auto res = db::pg::exec(qry, record.principalId(), record.resourceType(), record.resourceId());
	ASSERT_EQ(1, res.size());

	auto count = res.at(0, 0).as<int>();
	EXPECT_EQ(0, count);
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
	});
	ASSERT_NO_THROW(record.store());

	// Success: lookup record
	{
		auto result =
			db::LookupRecord(record.principalId(), record.resourceType(), record.resourceId());
		ASSERT_TRUE(result);

		EXPECT_EQ(record, result.value());
	}

	// Success: lookup non-existent record
	{
		auto result = db::LookupRecord(record.principalId(), record.resourceType(), "non-existent");
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
		});

		std::string_view qry = R"(
			insert into records (
				_rev,
				principal_id,
				resource_type,
				resource_id
			) values (
				$1::integer,
				$2::text,
				$3::text,
				$4::text
			)
		)";

		ASSERT_NO_THROW(db::pg::exec(
			qry,
			record.rev() + 1,
			record.principalId(),
			record.resourceType(),
			record.resourceId()));

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
		});
		ASSERT_NO_THROW(record.store());

		std::string_view qry = R"(
			select
				_rev,
				attrs
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

		auto [_rev, attrs] = res[0].as<int, db::Record::Data::attrs_t>();
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

	// Error: invalid `principalId`
	{
		db::Record record({
			.principalId  = "id:db_RecordsTest.store-invalid-principalId",
			.resourceId   = "store-invalid-principalId",
			.resourceType = "db_RecordsTest",
		});

		EXPECT_THROW(record.store(), err::DbRecordInvalidPrincipalId);
	}

	// Error: invalid `attrs`
	{
		db::Record record({
			.attrs        = R"("string")",
			.principalId  = principal.id(),
			.resourceId   = "store-invalid-attrs",
			.resourceType = "db_RecordsTest",
		});

		EXPECT_THROW(record.store(), err::DbRecordInvalidData);
	}
}
