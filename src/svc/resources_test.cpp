#include <google/protobuf/util/json_util.h>
#include <grpcxx/request.h>
#include <gtest/gtest.h>

#include "db/testing.h"

#include "common.h"
#include "svc.h"

using namespace sentium::api::v1::Resources;

class svc_ResourcesTest : public testing::Test {
protected:
	static void SetUpTestSuite() {
		db::testing::setup();

		// Clear data
		db::pg::exec("truncate table principals cascade;");
		db::pg::exec("truncate table tuples;");
	}

	static void TearDownTestSuite() { db::testing::teardown(); }
};

TEST_F(svc_ResourcesTest, List) {
	grpcxx::context ctx;
	svc::Resources  svc;

	db::Principal principal({.id = "id:svc_ResourcesTest.List"});
	ASSERT_NO_THROW(principal.store());

	// Success: list
	{
		db::Tuple tuple({
			.attrs        = R"({"foo":"bar"})",
			.lPrincipalId = principal.id(),
			.rEntityId    = "List",
			.rEntityType  = "svc_ResourcesTest",
			.spaceId      = principal.spaceId(),
		});
		ASSERT_NO_THROW(tuple.store());

		rpcList::request_type request;
		request.set_principal_id(principal.id());
		request.set_resource_type(tuple.rEntityType());

		rpcList::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcList>(ctx, request));
		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		ASSERT_TRUE(result.response);
		EXPECT_FALSE(result.response->has_pagination_token());

		auto &actual = result.response->resources();
		ASSERT_EQ(1, actual.size());
		EXPECT_EQ(tuple.rEntityId(), actual[0].id());
		EXPECT_EQ(tuple.rEntityType(), actual[0].type());

		std::string responseAttrs;
		google::protobuf::util::MessageToJsonString(actual[0].attrs(), &responseAttrs);
		EXPECT_EQ(tuple.attrs(), responseAttrs);
	}

	// Success: list with space-id
	{
		db::Principal principal({
			.id      = "id:svc_ResourcesTest.List-with_space_id",
			.spaceId = "space_id:svc_ResourcesTest.List-with_space_id",
		});
		ASSERT_NO_THROW(principal.store());

		db::Tuple tuple({
			.lPrincipalId = principal.id(),
			.rEntityId    = "List-with_space_id",
			.rEntityType  = "svc_ResourcesTest",
			.spaceId      = principal.spaceId(),
		});
		ASSERT_NO_THROW(tuple.store());

		grpcxx::detail::request r(1);
		r.header(std::string(svc::common::space_id_v), std::string(principal.spaceId()));

		grpcxx::context ctx(r);

		rpcList::request_type request;
		request.set_principal_id(principal.id());
		request.set_resource_type(tuple.rEntityType());

		rpcList::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcList>(ctx, request));
		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		ASSERT_TRUE(result.response);
		EXPECT_FALSE(result.response->has_pagination_token());

		auto &actual = result.response->resources();
		ASSERT_EQ(1, actual.size());
		EXPECT_EQ(tuple.rEntityId(), actual[0].id());
		EXPECT_EQ(tuple.rEntityType(), actual[0].type());
	}

	// Success: list with pagination
	{
		db::Tuples tuples({
			{{
				.lPrincipalId = principal.id(),
				.rEntityId    = "List-with_pagination[0]",
				.rEntityType  = "svc_ResourcesTest",
				.spaceId      = principal.spaceId(),
			}},
			{{
				.lPrincipalId = principal.id(),
				.rEntityId    = "List-with_pagination[1]",
				.rEntityType  = "svc_ResourcesTest",
				.spaceId      = principal.spaceId(),
			}},
		});

		for (auto &t : tuples) {
			ASSERT_NO_THROW(t.store());
		}

		rpcList::request_type request;
		request.set_principal_id(principal.id());
		request.set_resource_type(tuples[0].rEntityType());
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
			EXPECT_EQ(tuples[1].rEntityId(), actual[0].id());
			EXPECT_EQ(tuples[1].rEntityType(), actual[0].type());
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
			EXPECT_EQ(tuples[0].rEntityId(), actual[0].id());
			EXPECT_EQ(tuples[0].rEntityType(), actual[0].type());
			EXPECT_FALSE(actual[0].has_attrs());
		}
	}
}

TEST_F(svc_ResourcesTest, ListPrincipals) {
	grpcxx::context ctx;
	svc::Resources  svc;

	// Success: list
	{
		db::Principal principal({.id = "id:svc_ResourcesTest.ListPrincipals"});
		ASSERT_NO_THROW(principal.store());

		db::Tuple tuple({
			.attrs        = R"({"foo":"bar"})",
			.lPrincipalId = principal.id(),
			.rEntityId    = "ListPrincipals",
			.rEntityType  = "svc_ResourcesTest",
			.spaceId      = principal.spaceId(),
		});
		ASSERT_NO_THROW(tuple.store());

		rpcListPrincipals::request_type request;
		request.set_resource_id(tuple.rEntityId());
		request.set_resource_type(tuple.rEntityType());

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
		EXPECT_EQ(tuple.attrs(), responseAttrs);
	}

	// Success: list (space-id mismatch)
	{
		db::Principal principal({
			.id      = "id:svc_ResourcesTest.ListPrincipals-space_id_mismatch",
			.spaceId = "space_id:svc_ResourcesTest.ListPrincipals-space_id_mismatch",
		});
		ASSERT_NO_THROW(principal.store());

		db::Tuple tuple({
			.lPrincipalId = principal.id(),
			.rEntityId    = "ListPrincipals-space_id_mismatch",
			.rEntityType  = "svc_ResourcesTest",
			.spaceId      = principal.spaceId(),
		});
		ASSERT_NO_THROW(tuple.store());

		grpcxx::detail::request r(1);
		r.header(std::string(svc::common::space_id_v), "invalid");

		grpcxx::context ctx(r);

		rpcListPrincipals::request_type request;
		request.set_resource_id(tuple.rEntityId());
		request.set_resource_type(tuple.rEntityType());

		rpcListPrincipals::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcListPrincipals>(ctx, request));
		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		ASSERT_TRUE(result.response);
		EXPECT_FALSE(result.response->has_pagination_token());

		auto &actual = result.response->principals();
		ASSERT_EQ(0, actual.size());
	}

	// Success: list with pagination
	{
		db::Principals principals({
			{{.id = "id:svc_ResourcesTest.ListPrincipals-with_pagination[0]"}},
			{{.id = "id:svc_ResourcesTest.ListPrincipals-with_pagination[1]"}},
		});

		for (auto &p : principals) {
			ASSERT_NO_THROW(p.store());
		}

		db::Tuples tuples({
			{{
				.lPrincipalId = principals[0].id(),
				.rEntityId    = "ListPrincipals-with_pagination",
				.rEntityType  = "svc_ResourcesTest",
				.spaceId      = principals[0].spaceId(),
			}},
			{{
				.lPrincipalId = principals[1].id(),
				.rEntityId    = "ListPrincipals-with_pagination",
				.rEntityType  = "svc_ResourcesTest",
				.spaceId      = principals[1].spaceId(),
			}},
		});

		for (auto &r : tuples) {
			ASSERT_NO_THROW(r.store());
		}

		rpcListPrincipals::request_type request;
		request.set_resource_id(tuples[0].rEntityId());
		request.set_resource_type(tuples[0].rEntityType());
		request.set_pagination_limit(1);

		rpcListPrincipals::result_type result;

		// Page 1
		{
			EXPECT_NO_THROW(result = svc.call<rpcListPrincipals>(ctx, request));
			EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
			ASSERT_TRUE(result.response);

			EXPECT_TRUE(result.response->has_pagination_token());
			EXPECT_EQ(
				"18r6ip1qedr66nqiclpmutbicdin6l35edq2sj39edq50sj9dphmis31dhpiqtr9ehk5us31ctkmsobkd5"
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
				"18r6ip1qedr66nqiclpmutbicdin6l35edq2sj39edq50sj9dphmis31dhpiqtr9ehk5us31ctkmsobkd5"
				"nmsmpgbk",
				result.response->pagination_token());

			auto &actual = result.response->principals();
			ASSERT_EQ(1, actual.size());
			EXPECT_EQ(principals[0].id(), actual[0].id());
			EXPECT_FALSE(actual[0].has_attrs());
		}
	}
}
