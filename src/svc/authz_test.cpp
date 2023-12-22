#include <google/protobuf/util/json_util.h>
#include <gtest/gtest.h>

#include "db/testing.h"

#include "svc.h"

using namespace sentium::api::v1::Authz;

class svc_AuthzTest : public testing::Test {
protected:
	static void SetUpTestSuite() {
		db::testing::setup();

		// Clear data
		db::pg::exec("truncate table principals cascade;");
		db::pg::exec("truncate table records;");
	}

	static void TearDownTestSuite() { db::testing::teardown(); }
};

TEST_F(svc_AuthzTest, Grant) {
	grpcxx::context ctx;
	svc::Authz      svc;

	db::Principal principal({
		.id = "id:svc_AuthzTest.Grant",
	});
	ASSERT_NO_THROW(principal.store());

	// Success: grant
	{
		rpcGrant::request_type request;
		request.set_principal_id(principal.id());
		request.set_resource_type("svc_AuthzTest");
		request.set_resource_id("Grant");

		rpcGrant::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcGrant>(ctx, request));

		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		ASSERT_TRUE(result.response);
	}

	// Success: upsert
	{
		db::Record record({
			.principalId  = principal.id(),
			.resourceId   = "Grant-upsert",
			.resourceType = "svc_AuthzTest",
		});
		ASSERT_NO_THROW(record.store());

		rpcGrant::request_type request;
		request.set_principal_id(record.principalId());
		request.set_resource_type(record.resourceType());
		request.set_resource_id(record.resourceId());

		const std::string attrs(R"({"foo":"bar"})");
		google::protobuf::util::JsonStringToMessage(attrs, request.mutable_attrs());

		rpcGrant::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcGrant>(ctx, request));

		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		ASSERT_TRUE(result.response);

		auto actual =
			db::Record::lookup(record.principalId(), record.resourceType(), record.resourceId());
		EXPECT_EQ(record.rev() + 1, actual->rev());
		EXPECT_EQ(R"({"foo": "bar"})", actual->attrs());
	}

	// Error: invalid data
	{
		rpcGrant::request_type request;
		request.set_principal_id(principal.id());

		rpcGrant::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcGrant>(ctx, request));

		EXPECT_EQ(grpcxx::status::code_t::invalid_argument, result.status.code());
		ASSERT_FALSE(result.response);
	}

	// Error: invalid `principal_id`
	{
		rpcGrant::request_type request;
		request.set_principal_id("invalid");
		request.set_resource_type("svc_AuthzTest");
		request.set_resource_id("Grant-invalid:principal_id");

		rpcGrant::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcGrant>(ctx, request));

		EXPECT_EQ(grpcxx::status::code_t::invalid_argument, result.status.code());
		ASSERT_FALSE(result.response);
	}
}
