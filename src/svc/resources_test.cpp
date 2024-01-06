#include <google/protobuf/util/json_util.h>
#include <gtest/gtest.h>

#include "db/testing.h"

#include "svc.h"

using namespace sentium::api::v1::Resources;

class svc_ResourcesTest : public testing::Test {
protected:
	static void SetUpTestSuite() {
		db::testing::setup();

		// Clear data
		db::pg::exec("truncate table principals cascade;");
		db::pg::exec("truncate table records;");
	}

	static void TearDownTestSuite() { db::testing::teardown(); }
};

TEST_F(svc_ResourcesTest, List) {
	grpcxx::context ctx;
	svc::Resources  svc;

	db::Principal principal({.id = "id:svc_ResourcesTest-List"});
	ASSERT_NO_THROW(principal.store());

	// Success: list
	{
		db::Record record({
			.attrs        = R"({"foo":"bar"})",
			.principalId  = principal.id(),
			.resourceId   = "List",
			.resourceType = "svc_ResourcesTest",
		});
		ASSERT_NO_THROW(record.store());

		rpcList::request_type request;
		request.set_principal_id(principal.id());
		request.set_resource_type(record.resourceType());

		rpcList::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcList>(ctx, request));
		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		ASSERT_TRUE(result.response);
		EXPECT_FALSE(result.response->has_pagination_token());

		auto &actual = result.response->resources();
		ASSERT_EQ(1, actual.size());
		EXPECT_EQ(record.resourceId(), actual[0].id());
		EXPECT_EQ(record.resourceType(), actual[0].type());

		std::string responseAttrs;
		google::protobuf::util::MessageToJsonString(actual[0].attrs(), &responseAttrs);
		EXPECT_EQ(record.attrs(), responseAttrs);
	}

	// Success: list with pagination
	{
		db::Records records({
			{{
				.principalId  = principal.id(),
				.resourceId   = "List-with_pagination[0]",
				.resourceType = "svc_ResourcesTest",
			}},
			{{
				.principalId  = principal.id(),
				.resourceId   = "List-with_pagination[1]",
				.resourceType = "svc_ResourcesTest",
			}},
		});

		for (auto &r : records) {
			ASSERT_NO_THROW(r.store());
		}

		rpcList::request_type request;
		request.set_principal_id(principal.id());
		request.set_resource_type(records[0].resourceType());
		request.set_pagination_limit(1);

		rpcList::result_type result;

		// Page 1
		{
			EXPECT_NO_THROW(result = svc.call<rpcList>(ctx, request));
			EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
			ASSERT_TRUE(result.response);

			EXPECT_TRUE(result.response->has_pagination_token());
			EXPECT_EQ(
				"18bkoqbjegmneqbkd1fn0ob7d5n62t39dtn5mcat", result.response->pagination_token());

			auto &actual = result.response->resources();
			ASSERT_EQ(1, actual.size());
			EXPECT_EQ(records[1].resourceId(), actual[0].id());
			EXPECT_EQ(records[1].resourceType(), actual[0].type());
			EXPECT_FALSE(actual[0].has_attrs());
		}

		// Use pagination token to get the next page of results
		request.set_pagination_token(result.response->pagination_token());

		// Page 2
		{
			EXPECT_NO_THROW(result = svc.call<rpcList>(ctx, request));
			EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
			ASSERT_TRUE(result.response);

			EXPECT_TRUE(result.response->has_pagination_token());
			EXPECT_EQ(
				"18bkoqbjegmneqbkd1fn0ob7d5n62t39dtn5mc2t", result.response->pagination_token());

			auto &actual = result.response->resources();
			ASSERT_EQ(1, actual.size());
			EXPECT_EQ(records[0].resourceId(), actual[0].id());
			EXPECT_EQ(records[0].resourceType(), actual[0].type());
			EXPECT_FALSE(actual[0].has_attrs());
		}
	}
}

TEST_F(svc_ResourcesTest, ListPrincipals) {
	grpcxx::context ctx;
	svc::Resources  svc;

	// Success: list
	{
		db::Principal principal({.id = "id:svc_ResourcesTest-ListPrincipals"});
		ASSERT_NO_THROW(principal.store());

		db::Record record({
			.attrs        = R"({"foo":"bar"})",
			.principalId  = principal.id(),
			.resourceId   = "ListPrincipals",
			.resourceType = "svc_ResourcesTest",
		});
		ASSERT_NO_THROW(record.store());

		rpcListPrincipals::request_type request;
		request.set_resource_id(record.resourceId());
		request.set_resource_type(record.resourceType());

		rpcListPrincipals::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcListPrincipals>(ctx, request));
		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		ASSERT_TRUE(result.response);
		EXPECT_FALSE(result.response->has_pagination_token());

		auto &actual = result.response->principals();
		ASSERT_EQ(1, actual.size());
		EXPECT_EQ(principal.id(), actual[0].id());

		std::string responseAttrs;
		google::protobuf::util::MessageToJsonString(actual[0].attrs(), &responseAttrs);
		EXPECT_EQ(record.attrs(), responseAttrs);
	}

	// Success: list with pagination
	{
		db::Principals principals({
			{{.id = "id:svc_ResourcesTest-ListPrincipals-with_pagination[0]"}},
			{{.id = "id:svc_ResourcesTest-ListPrincipals-with_pagination[1]"}},
		});

		for (auto &p : principals) {
			ASSERT_NO_THROW(p.store());
		}

		db::Records records({
			{{
				.principalId  = principals[0].id(),
				.resourceId   = "ListPrincipals-with_pagination",
				.resourceType = "svc_ResourcesTest",
			}},
			{{
				.principalId  = principals[1].id(),
				.resourceId   = "ListPrincipals-with_pagination",
				.resourceType = "svc_ResourcesTest",
			}},
		});

		for (auto &r : records) {
			ASSERT_NO_THROW(r.store());
		}

		rpcListPrincipals::request_type request;
		request.set_resource_id(records[0].resourceId());
		request.set_resource_type(records[0].resourceType());
		request.set_pagination_limit(1);

		rpcListPrincipals::result_type result;

		// Page 1
		{
			EXPECT_NO_THROW(result = svc.call<rpcListPrincipals>(ctx, request));
			EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
			ASSERT_TRUE(result.response);

			EXPECT_TRUE(result.response->has_pagination_token());
			EXPECT_EQ(
				"18r6ip1qedr66nqiclpmutbicdin6l35edq2qj39edq50sj9dphmis31dhpiqtr9ehk5us31ctkmsobkd5"
				"nmsmphbk",
				result.response->pagination_token());

			auto &actual = result.response->principals();
			ASSERT_EQ(1, actual.size());
			EXPECT_EQ(principals[1].id(), actual[0].id());
			EXPECT_FALSE(actual[0].has_attrs());
		}

		// Use pagination token to get the next page of results
		request.set_pagination_token(result.response->pagination_token());

		// Page 2
		{
			EXPECT_NO_THROW(result = svc.call<rpcListPrincipals>(ctx, request));
			EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
			ASSERT_TRUE(result.response);

			EXPECT_TRUE(result.response->has_pagination_token());
			EXPECT_EQ(
				"18r6ip1qedr66nqiclpmutbicdin6l35edq2qj39edq50sj9dphmis31dhpiqtr9ehk5us31ctkmsobkd5"
				"nmsmpgbk",
				result.response->pagination_token());

			auto &actual = result.response->principals();
			ASSERT_EQ(1, actual.size());
			EXPECT_EQ(principals[0].id(), actual[0].id());
			EXPECT_FALSE(actual[0].has_attrs());
		}
	}
}
