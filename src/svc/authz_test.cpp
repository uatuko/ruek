#include <google/protobuf/util/json_util.h>
#include <grpcxx/request.h>
#include <gtest/gtest.h>

#include "db/testing.h"

#include "common.h"
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

TEST_F(svc_AuthzTest, Check) {
	grpcxx::context ctx;
	svc::Authz      svc;

	db::Principal principal({
		.id = "id:svc_AuthzTest.Check",
	});
	ASSERT_NO_THROW(principal.store());

	// Success: check
	{
		db::Record record({
			.principalId  = principal.id(),
			.resourceId   = "Check",
			.resourceType = "svc_AuthzTest",
			.spaceId      = principal.spaceId(),
		});
		ASSERT_NO_THROW(record.store());

		rpcCheck::request_type request;
		request.set_principal_id(record.principalId());
		request.set_resource_type(record.resourceType());
		request.set_resource_id(record.resourceId());

		rpcCheck::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcCheck>(ctx, request));
		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		ASSERT_TRUE(result.response);

		EXPECT_TRUE(result.response->ok());
		EXPECT_FALSE(result.response->has_attrs());
	}

	// Success: check with `attrs`
	{
		db::Record record({
			.attrs        = R"({"foo":"bar"})",
			.principalId  = principal.id(),
			.resourceId   = "Check-with_attrs",
			.resourceType = "svc_AuthzTest",
			.spaceId      = principal.spaceId(),
		});
		ASSERT_NO_THROW(record.store());

		rpcCheck::request_type request;
		request.set_principal_id(record.principalId());
		request.set_resource_type(record.resourceType());
		request.set_resource_id(record.resourceId());

		rpcCheck::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcCheck>(ctx, request));
		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		ASSERT_TRUE(result.response);

		EXPECT_TRUE(result.response->ok());

		std::string responseAttrs;
		google::protobuf::util::MessageToJsonString(result.response->attrs(), &responseAttrs);
		EXPECT_EQ(record.attrs(), responseAttrs);
	}

	// Success: check with space-id
	{
		db::Principal principal({
			.id      = "id:svc_AuthzTest.Check",
			.spaceId = "space_id:svc_AuthzTest.Check",
		});
		ASSERT_NO_THROW(principal.store());

		db::Record record({
			.principalId  = principal.id(),
			.resourceId   = "Check-with_space_id",
			.resourceType = "svc_AuthzTest",
			.spaceId      = principal.spaceId(),
		});
		ASSERT_NO_THROW(record.store());

		grpcxx::detail::request r(1);
		r.header(std::string(svc::common::space_id_v), std::string(principal.spaceId()));

		grpcxx::context ctx(r);

		rpcCheck::request_type request;
		request.set_principal_id(record.principalId());
		request.set_resource_type(record.resourceType());
		request.set_resource_id(record.resourceId());

		rpcCheck::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcCheck>(ctx, request));
		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		ASSERT_TRUE(result.response);

		EXPECT_TRUE(result.response->ok());
		EXPECT_FALSE(result.response->has_attrs());
	}

	// Success: check !ok
	{
		rpcCheck::request_type request;
		request.set_principal_id("non-existent");
		request.set_resource_type("svc_AuthzTest");
		request.set_resource_id("Check-non_existent");

		rpcCheck::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcCheck>(ctx, request));
		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		ASSERT_TRUE(result.response);

		EXPECT_FALSE(result.response->ok());
		EXPECT_FALSE(result.response->has_attrs());
	}

	// Success: check !ok with space-id
	{
		db::Record record({
			.principalId  = principal.id(),
			.resourceId   = "Check-space_id_mismatch",
			.resourceType = "svc_AuthzTest",
			.spaceId      = principal.spaceId(),
		});
		ASSERT_NO_THROW(record.store());

		grpcxx::detail::request r(1);
		r.header(std::string(svc::common::space_id_v), "invalid");

		grpcxx::context ctx(r);

		rpcCheck::request_type request;
		request.set_principal_id(record.principalId());
		request.set_resource_type(record.resourceType());
		request.set_resource_id(record.resourceId());

		rpcCheck::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcCheck>(ctx, request));
		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		ASSERT_TRUE(result.response);

		EXPECT_FALSE(result.response->ok());
		EXPECT_FALSE(result.response->has_attrs());
	}
}

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

	// Success: grant with space-id
	{
		db::Principal principal({
			.id      = "id:svc_AuthzTest.Grant-with_space_id",
			.spaceId = "space_id:svc_AuthzTest.Grant-with_space_id",
		});
		ASSERT_NO_THROW(principal.store());

		grpcxx::detail::request r(1);
		r.header(std::string(svc::common::space_id_v), std::string(principal.spaceId()));

		grpcxx::context ctx(r);

		rpcGrant::request_type request;
		request.set_principal_id(principal.id());
		request.set_resource_type("svc_AuthzTest");
		request.set_resource_id("Grant-with_space_id");

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
			.spaceId      = principal.spaceId(),
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

		auto actual = db::Record::lookup(
			record.spaceId(), record.principalId(), record.resourceType(), record.resourceId());
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
		request.set_resource_id("Grant-invalid_principal_id");

		rpcGrant::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcGrant>(ctx, request));

		EXPECT_EQ(grpcxx::status::code_t::invalid_argument, result.status.code());
		ASSERT_FALSE(result.response);
	}

	// Error: invalid space-id
	{
		grpcxx::detail::request r(1);
		r.header(std::string(svc::common::space_id_v), "invalid");

		grpcxx::context ctx(r);

		rpcGrant::request_type request;
		request.set_principal_id(principal.id());
		request.set_resource_type("svc_AuthzTest");
		request.set_resource_id("Grant-invalid_space_id");

		rpcGrant::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcGrant>(ctx, request));

		EXPECT_EQ(grpcxx::status::code_t::invalid_argument, result.status.code());
		ASSERT_FALSE(result.response);
	}
}

TEST_F(svc_AuthzTest, Revoke) {
	grpcxx::context ctx;
	svc::Authz      svc;

	db::Principal principal({
		.id = "id:svc_AuthzTest.Revoke",
	});
	ASSERT_NO_THROW(principal.store());

	// Success: revoke
	{
		db::Record record({
			.principalId  = principal.id(),
			.resourceId   = "Revoke",
			.resourceType = "svc_AuthzTest",
			.spaceId      = principal.spaceId(),
		});
		ASSERT_NO_THROW(record.store());

		rpcRevoke::request_type request;
		request.set_principal_id(record.principalId());
		request.set_resource_type(record.resourceType());
		request.set_resource_id(record.resourceId());

		rpcRevoke::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcRevoke>(ctx, request));

		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		ASSERT_TRUE(result.response);

		EXPECT_FALSE(db::Record::lookup(
			record.spaceId(), record.principalId(), record.resourceType(), record.resourceId()));
	}

	// Success: invalid space-id
	{
		db::Record record({
			.principalId  = principal.id(),
			.resourceId   = "Revoke-invalid_space_id",
			.resourceType = "svc_AuthzTest",
			.spaceId      = principal.spaceId(),
		});
		ASSERT_NO_THROW(record.store());

		grpcxx::detail::request r(1);
		r.header(std::string(svc::common::space_id_v), "invalid");

		grpcxx::context ctx(r);

		rpcRevoke::request_type request;
		request.set_principal_id(record.principalId());
		request.set_resource_type(record.resourceType());
		request.set_resource_id(record.resourceId());

		rpcRevoke::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcRevoke>(ctx, request));

		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		ASSERT_TRUE(result.response);

		EXPECT_TRUE(db::Record::lookup(
			record.spaceId(), record.principalId(), record.resourceType(), record.resourceId()));
	}
}
