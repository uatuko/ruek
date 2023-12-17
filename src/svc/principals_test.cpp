#include <google/protobuf/util/json_util.h>
#include <gtest/gtest.h>

#include "db/testing.h"

#include "svc.h"

using namespace sentium::api::v1::Principals;

class svc_PrincipalsTest : public testing::Test {
protected:
	static void SetUpTestSuite() {
		db::testing::setup();

		// Clear data
		db::pg::exec("truncate table principals cascade;");
	}

	static void TearDownTestSuite() { db::testing::teardown(); }
};

TEST_F(svc_PrincipalsTest, Create) {
	grpcxx::context ctx;
	svc::Principals svc;

	// Success: create principal
	{
		rpcCreate::request_type request;

		rpcCreate::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcCreate>(ctx, request));

		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		ASSERT_TRUE(result.response);
		EXPECT_FALSE(result.response->id().empty());
	}

	// Success: create principal with `id`
	{
		rpcCreate::request_type request;
		request.set_id("id:svc_PrincipalsTest.Create-with_id");

		rpcCreate::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcCreate>(ctx, request));

		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		ASSERT_TRUE(result.response);
		EXPECT_EQ(request.id(), result.response->id());
	}

	// Success: create principal with `attrs`
	{
		rpcCreate::request_type request;

		const std::string attrs(R"({"foo":"bar"})");
		google::protobuf::util::JsonStringToMessage(attrs, request.mutable_attrs());

		rpcCreate::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcCreate>(ctx, request));

		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		ASSERT_TRUE(result.response);

		std::string responseAttrs;
		google::protobuf::util::MessageToJsonString(result.response->attrs(), &responseAttrs);
		EXPECT_EQ(attrs, responseAttrs);
	}

	// Error: invalid `parent_id`
	{
		rpcCreate::request_type request;
		request.set_parent_id("dummy");

		rpcCreate::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcCreate>(ctx, request));

		EXPECT_EQ(grpcxx::status::code_t::invalid_argument, result.status.code());
		EXPECT_FALSE(result.response);
	}

	// Error: duplicate `id`
	{
		db::Principal p({
			.id = "id:svc_PrincipalsTest.Create-duplicate_id",
		});
		ASSERT_NO_THROW(p.store());

		rpcCreate::request_type request;
		request.set_id(p.id());

		rpcCreate::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcCreate>(ctx, request));

		EXPECT_EQ(grpcxx::status::code_t::already_exists, result.status.code());
		EXPECT_FALSE(result.response);
	}
}
