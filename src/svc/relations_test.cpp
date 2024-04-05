#include <google/protobuf/util/json_util.h>
#include <grpcxx/request.h>
#include <gtest/gtest.h>

#include "db/testing.h"

#include "common.h"
#include "svc.h"

using namespace sentium::api::v1::Relations;

class svc_RelationsTest : public testing::Test {
protected:
	static void SetUpTestSuite() {
		db::testing::setup();

		// Clear data
		db::pg::exec("truncate table principals cascade;");
		db::pg::exec("truncate table tuples;");
	}

	static void TearDownTestSuite() { db::testing::teardown(); }
};

TEST_F(svc_RelationsTest, Check) {
	grpcxx::context ctx;
	svc::Relations  svc;

	// Success: found
	{
		db::Tuple tuple({
			.lEntityId   = "left",
			.lEntityType = "svc_RelationsTest.Check",
			.relation    = "relation",
			.rEntityId   = "right",
			.rEntityType = "svc_RelationsTest.Check",
		});
		ASSERT_NO_THROW(tuple.store());

		rpcCheck::request_type request;

		auto *left = request.mutable_left_entity();
		left->set_id(tuple.lEntityId());
		left->set_type(tuple.lEntityType());

		request.set_relation(tuple.relation());

		auto *right = request.mutable_right_entity();
		right->set_id(tuple.rEntityId());
		right->set_type(tuple.rEntityType());

		rpcCheck::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcCheck>(ctx, request));

		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		ASSERT_TRUE(result.response);
		EXPECT_EQ(true, result.response->found());
		EXPECT_EQ(1, result.response->cost());
		ASSERT_TRUE(result.response->has_tuple());

		auto &actual = result.response->tuple();
		EXPECT_EQ(tuple.spaceId(), actual.space_id());
		EXPECT_EQ(tuple.id(), actual.id());
		EXPECT_FALSE(actual.has_left_principal_id());
		EXPECT_EQ(tuple.lEntityId(), actual.left_entity().id());
		EXPECT_EQ(tuple.lEntityType(), actual.left_entity().type());
		EXPECT_EQ(tuple.relation(), actual.relation());
		EXPECT_FALSE(actual.has_right_principal_id());
		EXPECT_EQ(tuple.rEntityId(), actual.right_entity().id());
		EXPECT_EQ(tuple.rEntityType(), actual.right_entity().type());
		EXPECT_FALSE(actual.has_strand());
		EXPECT_FALSE(actual.has_attrs());
		EXPECT_FALSE(actual.has_ref_id());
	}

	// Success: found with principals
	{
		db::Principal left({.id = "id:svc_RelationsTest.Check-with_principals_left"});
		ASSERT_NO_THROW(left.store());

		db::Principal right({.id = "id:svc_RelationsTest.Check-with_principals_right"});
		ASSERT_NO_THROW(right.store());

		db::Tuple tuple({
			.lPrincipalId = left.id(),
			.relation     = "relation",
			.rPrincipalId = right.id(),
		});
		ASSERT_NO_THROW(tuple.store());

		rpcCheck::request_type request;
		request.set_left_principal_id(*tuple.lPrincipalId());
		request.set_relation(tuple.relation());
		request.set_right_principal_id(*tuple.rPrincipalId());

		rpcCheck::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcCheck>(ctx, request));

		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		ASSERT_TRUE(result.response);
		EXPECT_EQ(true, result.response->found());
		EXPECT_EQ(1, result.response->cost());
		ASSERT_TRUE(result.response->has_tuple());

		auto &actual = result.response->tuple();
		EXPECT_EQ(tuple.spaceId(), actual.space_id());
		EXPECT_EQ(tuple.id(), actual.id());
		EXPECT_FALSE(actual.has_left_entity());
		EXPECT_EQ(*tuple.lPrincipalId(), actual.left_principal_id());
		EXPECT_EQ(tuple.relation(), actual.relation());
		EXPECT_FALSE(actual.has_right_entity());
		EXPECT_EQ(*tuple.rPrincipalId(), actual.right_principal_id());
		EXPECT_FALSE(actual.has_strand());
		EXPECT_FALSE(actual.has_attrs());
		EXPECT_FALSE(actual.has_ref_id());
	}

	// Success: not found (space-id mismatch)
	{
		db::Tuple tuple({
			.lEntityId   = "left",
			.lEntityType = "svc_RelationsTest.Check-not_found",
			.relation    = "relation",
			.rEntityId   = "right",
			.rEntityType = "svc_RelationsTest.Check-not_found",
		});
		ASSERT_NO_THROW(tuple.store());

		rpcCheck::request_type request;

		auto *left = request.mutable_left_entity();
		left->set_id(tuple.lEntityId());
		left->set_type(tuple.lEntityType());

		request.set_relation(tuple.relation());

		auto *right = request.mutable_right_entity();
		right->set_id(tuple.rEntityId());
		right->set_type(tuple.rEntityType());

		grpcxx::detail::request r(1);
		r.header(
			std::string(svc::common::space_id_v), "space_id:svc_RelationsTest.Check-not_found");

		grpcxx::context ctx(r);

		rpcCheck::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcCheck>(ctx, request));

		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		ASSERT_TRUE(result.response);
		EXPECT_EQ(false, result.response->found());
		EXPECT_EQ(1, result.response->cost());
		EXPECT_FALSE(result.response->has_tuple());
	}
}

TEST_F(svc_RelationsTest, Create) {
	grpcxx::context ctx;
	svc::Relations  svc;

	// Success: create relation
	{
		rpcCreate::request_type request;

		auto *left = request.mutable_left_entity();
		left->set_id("left");
		left->set_type("svc_RelationsTest.Create");

		request.set_relation("relation");

		auto *right = request.mutable_right_entity();
		right->set_id("right");
		right->set_type("svc_RelationsTest.Create");

		request.set_strand("strand");

		const std::string attrs(R"({"foo":"bar"})");
		google::protobuf::util::JsonStringToMessage(attrs, request.mutable_attrs());

		rpcCreate::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcCreate>(ctx, request));

		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		ASSERT_TRUE(result.response);
		EXPECT_EQ(1, result.response->cost());

		auto &actual = result.response->tuple();
		EXPECT_FALSE(actual.id().empty());
		EXPECT_EQ("", actual.space_id());

		EXPECT_FALSE(actual.has_left_principal_id());
		EXPECT_EQ(left->id(), actual.left_entity().id());
		EXPECT_EQ(left->type(), actual.left_entity().type());
		EXPECT_EQ(request.relation(), actual.relation());
		EXPECT_FALSE(actual.has_right_principal_id());
		EXPECT_EQ(right->id(), actual.right_entity().id());
		EXPECT_EQ(right->type(), actual.right_entity().type());
		EXPECT_EQ(request.strand(), actual.strand());

		std::string responseAttrs;
		google::protobuf::util::MessageToJsonString(actual.attrs(), &responseAttrs);
		EXPECT_EQ(attrs, responseAttrs);

		EXPECT_FALSE(actual.has_ref_id());
	}

	// Success: create relation with principals
	{
		db::Principal principal({.id = "svc_RelationsTest.Create-with_principals"});
		ASSERT_NO_THROW(principal.store());

		rpcCreate::request_type request;
		request.set_left_principal_id(principal.id());
		request.set_relation("relation");
		request.set_right_principal_id(principal.id());

		rpcCreate::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcCreate>(ctx, request));

		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		ASSERT_TRUE(result.response);
		EXPECT_EQ(1, result.response->cost());

		auto &actual = result.response->tuple();
		EXPECT_FALSE(actual.id().empty());
		EXPECT_EQ("", actual.space_id());

		EXPECT_FALSE(actual.has_left_entity());
		EXPECT_EQ(principal.id(), actual.left_principal_id());
		EXPECT_EQ(request.relation(), actual.relation());
		EXPECT_FALSE(actual.has_right_entity());
		EXPECT_EQ(principal.id(), actual.right_principal_id());

		EXPECT_FALSE(actual.has_strand());
		EXPECT_FALSE(actual.has_attrs());
		EXPECT_FALSE(actual.has_ref_id());
	}

	// Success: create relation with space-id
	{
		grpcxx::detail::request r(1);
		r.header(
			std::string(svc::common::space_id_v),
			"space_id:svc_RelationsTest.Create-with_space_id");

		grpcxx::context ctx(r);

		rpcCreate::request_type request;

		auto *left = request.mutable_left_entity();
		left->set_id("left");
		left->set_type("svc_RelationsTest.Create-with_space_id");

		request.set_relation("relation");

		auto *right = request.mutable_right_entity();
		right->set_id("right");
		right->set_type("svc_RelationsTest.Create-with_space_id");

		rpcCreate::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcCreate>(ctx, request));

		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		ASSERT_TRUE(result.response);
		EXPECT_EQ(1, result.response->cost());

		auto &actual = result.response->tuple();
		EXPECT_FALSE(actual.id().empty());
		EXPECT_EQ("space_id:svc_RelationsTest.Create-with_space_id", actual.space_id());

		EXPECT_FALSE(actual.has_left_principal_id());
		EXPECT_EQ(left->id(), actual.left_entity().id());
		EXPECT_EQ(left->type(), actual.left_entity().type());
		EXPECT_EQ(request.relation(), actual.relation());
		EXPECT_FALSE(actual.has_right_principal_id());
		EXPECT_EQ(right->id(), actual.right_entity().id());
		EXPECT_EQ(right->type(), actual.right_entity().type());

		EXPECT_FALSE(actual.has_strand());
		EXPECT_FALSE(actual.has_attrs());
		EXPECT_FALSE(actual.has_ref_id());
	}

	// Error: invalid entity
	{
		rpcCreate::request_type request;

		auto *left = request.mutable_left_entity();
		left->set_id("left");
		left->set_type("svc_RelationsTest.Create-invalid_entity");

		request.set_relation("relation");

		auto *right = request.mutable_right_entity(); // empty id
		right->set_type("svc_RelationsTest.Create-invalid_entity");

		rpcCreate::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcCreate>(ctx, request));

		EXPECT_EQ(grpcxx::status::code_t::invalid_argument, result.status.code());
		ASSERT_FALSE(result.response);
	}

	// Error: invalid principal
	{
		db::Principal principal({.spaceId = "svc_RelationsTest.Create--invalid-principal"});
		ASSERT_NO_THROW(principal.store());

		rpcCreate::request_type request;
		request.set_left_principal_id(principal.id()); // space-id mismatch
		request.set_relation("relation");

		auto *right = request.mutable_right_entity();
		right->set_id("right");
		right->set_type("svc_RelationsTest.Create-invalid-principal");

		rpcCreate::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcCreate>(ctx, request));

		EXPECT_EQ(grpcxx::status::code_t::invalid_argument, result.status.code());
		ASSERT_FALSE(result.response);
	}
}
