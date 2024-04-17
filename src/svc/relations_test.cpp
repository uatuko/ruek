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
			.strand      = "strand",
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
		EXPECT_EQ(tuple.strand(), actual.strand());
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
		db::Principal principal({.spaceId = "svc_RelationsTest.Create-invalid-principal"});
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

TEST_F(svc_RelationsTest, Delete) {
	grpcxx::context ctx;
	svc::Relations  svc;

	// Success: delete
	{
		db::Tuple tuple({
			.lEntityId   = "left",
			.lEntityType = "svc_RelationsTest.Delete",
			.relation    = "relation",
			.rEntityId   = "right",
			.rEntityType = "svc_RelationsTest.Delete",
		});
		ASSERT_NO_THROW(tuple.store());

		rpcDelete::request_type request;

		auto *left = request.mutable_left_entity();
		left->set_id(tuple.lEntityId());
		left->set_type(tuple.lEntityType());

		request.set_relation(tuple.relation());

		auto *right = request.mutable_right_entity();
		right->set_id(tuple.rEntityId());
		right->set_type(tuple.rEntityType());

		rpcDelete::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcDelete>(ctx, request));
		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		EXPECT_TRUE(result.response);

		EXPECT_FALSE(db::Tuple::discard(tuple.id()));
	}

	// Success: delete with principals
	{
		std::string_view spaceId = "space_id:svc_RelationsTest.Delete-with_principals";

		db::Principal left({
			.id      = "id:ssvc_RelationsTest.Delete-with_principals_left",
			.spaceId = std::string(spaceId),
		});
		ASSERT_NO_THROW(left.store());

		db::Principal right({
			.id      = "id:svc_RelationsTest.Delete-with_principals_right",
			.spaceId = std::string(spaceId),
		});
		ASSERT_NO_THROW(right.store());

		db::Tuple tuple({
			.lPrincipalId = left.id(),
			.relation     = "relation",
			.rPrincipalId = right.id(),
			.spaceId      = std::string(spaceId),
			.strand       = "strand",
		});
		ASSERT_NO_THROW(tuple.store());

		rpcDelete::request_type request;
		request.set_left_principal_id(*tuple.lPrincipalId());
		request.set_relation(tuple.relation());
		request.set_right_principal_id(*tuple.rPrincipalId());
		request.set_strand(tuple.strand());

		grpcxx::detail::request r(1);
		r.header(std::string(svc::common::space_id_v), std::string(spaceId));

		grpcxx::context ctx(r);

		rpcDelete::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcDelete>(ctx, request));
		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		EXPECT_TRUE(result.response);

		EXPECT_FALSE(db::Tuple::discard(tuple.id()));
	}

	// Success: not found (strand mismatch)
	{
		db::Tuple tuple({
			.lEntityId   = "left",
			.lEntityType = "svc_RelationsTest.Delete-not_found",
			.relation    = "relation",
			.rEntityId   = "right",
			.rEntityType = "svc_RelationsTest.Delete-not_found",
			.strand      = "strand",
		});
		ASSERT_NO_THROW(tuple.store());

		rpcDelete::request_type request;

		auto *left = request.mutable_left_entity();
		left->set_id(tuple.lEntityId());
		left->set_type(tuple.lEntityType());

		request.set_relation(tuple.relation());

		auto *right = request.mutable_right_entity();
		right->set_id(tuple.rEntityId());
		right->set_type(tuple.rEntityType());

		rpcDelete::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcDelete>(ctx, request));
		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		EXPECT_TRUE(result.response);

		EXPECT_TRUE(db::Tuple::discard(tuple.id()));
	}
}

TEST_F(svc_RelationsTest, ListLeft) {
	grpcxx::context ctx;
	svc::Relations  svc;

	// Success: list left
	{
		db::Tuple tuple({
			.lEntityId   = "left",
			.lEntityType = "svc_RelationsTest.list",
			.relation    = "relation",
			.rEntityId   = "right",
			.rEntityType = "svc_RelationsTest.list-left",
		});
		ASSERT_NO_THROW(tuple.store());

		rpcListLeft::request_type request;

		auto *right = request.mutable_right_entity();
		right->set_id(tuple.rEntityId());
		right->set_type(tuple.rEntityType());

		rpcListLeft::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcListLeft>(ctx, request));
		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		ASSERT_TRUE(result.response);

		EXPECT_FALSE(result.response->has_pagination_token());

		auto &actual = result.response->tuples();
		ASSERT_EQ(1, actual.size());
		EXPECT_EQ(tuple.spaceId(), actual[0].space_id());
		EXPECT_EQ(tuple.id(), actual[0].id());
		EXPECT_FALSE(actual[0].has_left_principal_id());
		EXPECT_EQ(tuple.lEntityId(), actual[0].left_entity().id());
		EXPECT_EQ(tuple.lEntityType(), actual[0].left_entity().type());
		EXPECT_EQ(tuple.relation(), actual[0].relation());
		EXPECT_FALSE(actual[0].has_right_principal_id());
		EXPECT_EQ(tuple.rEntityId(), actual[0].right_entity().id());
		EXPECT_EQ(tuple.rEntityType(), actual[0].right_entity().type());
		EXPECT_FALSE(actual[0].has_strand());
		EXPECT_FALSE(actual[0].has_attrs());
		EXPECT_FALSE(actual[0].has_ref_id());
	}

	// Success: list left with relation
	{
		db::Tuples tuples({
			{{
				.lEntityId   = "left",
				.lEntityType = "svc_RelationsTest.list",
				.relation    = "relation[0]",
				.rEntityId   = "right",
				.rEntityType = "svc_RelationsTest.list-left_with_relation",
			}},
			{{
				.lEntityId   = "left",
				.lEntityType = "svc_RelationsTest.list",
				.relation    = "relation[1]",
				.rEntityId   = "right",
				.rEntityType = "svc_RelationsTest.list-left_with_relation",
			}},
		});

		for (auto &t : tuples) {
			ASSERT_NO_THROW(t.store());
		}

		rpcListLeft::request_type request;

		auto *right = request.mutable_right_entity();
		right->set_id(tuples[0].rEntityId());
		right->set_type(tuples[0].rEntityType());

		request.set_relation(tuples[0].relation());

		rpcListLeft::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcListLeft>(ctx, request));
		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		ASSERT_TRUE(result.response);

		EXPECT_FALSE(result.response->has_pagination_token());

		auto &actual = result.response->tuples();
		ASSERT_EQ(1, actual.size());
		EXPECT_EQ(tuples[0].id(), actual[0].id());
	}

	// Success: list left with space-id
	{
		db::Tuples tuples({
			{{
				.lEntityId   = "left",
				.lEntityType = "svc_RelationsTest.list",
				.relation    = "relation",
				.rEntityId   = "right",
				.rEntityType = "svc_RelationsTest.list-left_with_space_id",
				.spaceId     = "space_id[0]",
			}},
			{{
				.lEntityId   = "left",
				.lEntityType = "svc_RelationsTest.list",
				.relation    = "relation",
				.rEntityId   = "right",
				.rEntityType = "svc_RelationsTest.list-left_with_space_id",
				.spaceId     = "space_id[1]",
			}},
		});

		for (auto &t : tuples) {
			ASSERT_NO_THROW(t.store());
		}

		grpcxx::detail::request r(1);
		r.header(std::string(svc::common::space_id_v), std::string(tuples[1].spaceId()));

		grpcxx::context ctx(r);

		rpcListLeft::request_type request;

		auto *right = request.mutable_right_entity();
		right->set_id(tuples[1].rEntityId());
		right->set_type(tuples[1].rEntityType());

		rpcListLeft::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcListLeft>(ctx, request));
		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		ASSERT_TRUE(result.response);

		EXPECT_FALSE(result.response->has_pagination_token());

		auto &actual = result.response->tuples();
		ASSERT_EQ(1, actual.size());
		EXPECT_EQ(tuples[1].id(), actual[0].id());
	}

	// Success: list left with pagination
	{
		db::Tuples tuples({
			{{
				.lEntityId   = "left[0]",
				.lEntityType = "svc_RelationsTest.list",
				.relation    = "relation",
				.rEntityId   = "right",
				.rEntityType = "svc_RelationsTest.list-left_with_pagination",
			}},
			{{
				.lEntityId   = "left[1]",
				.lEntityType = "svc_RelationsTest.list",
				.relation    = "relation",
				.rEntityId   = "right",
				.rEntityType = "svc_RelationsTest.list-left_with_pagination",
			}},
		});

		for (auto &t : tuples) {
			ASSERT_NO_THROW(t.store());
		}

		rpcListLeft::request_type request;
		request.set_pagination_limit(1);

		auto *right = request.mutable_right_entity();
		right->set_id(tuples[0].rEntityId());
		right->set_type(tuples[0].rEntityType());

		rpcListLeft::result_type result;

		// Page 1
		{
			EXPECT_NO_THROW(result = svc.call<rpcListLeft>(ctx, request));
			EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
			ASSERT_TRUE(result.response);

			ASSERT_TRUE(result.response->has_pagination_token());
			EXPECT_EQ("183mopb6ehdj2n8", result.response->pagination_token());

			auto &actual = result.response->tuples();
			ASSERT_EQ(1, actual.size());
			EXPECT_EQ(tuples[1].id(), actual[0].id());
		}

		// Use pagination token to get the next page of results
		request.set_pagination_token(result.response->pagination_token());

		// Page 2
		{
			EXPECT_NO_THROW(result = svc.call<rpcListLeft>(ctx, request));
			EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
			ASSERT_TRUE(result.response);

			ASSERT_TRUE(result.response->has_pagination_token());
			EXPECT_EQ("183mopb6ehdj0n8", result.response->pagination_token());

			auto &actual = result.response->tuples();
			ASSERT_EQ(1, actual.size());
			EXPECT_EQ(tuples[0].id(), actual[0].id());
		}
	}

	// Success: list with invalid pagination token
	{
		db::Tuple tuple({
			.lEntityId   = "left",
			.lEntityType = "svc_RelationsTest.list",
			.relation    = "relation",
			.rEntityId   = "right",
			.rEntityType = "svc_RelationsTest.list-left_with_invalid_pagination_token",
		});
		ASSERT_NO_THROW(tuple.store());

		rpcListLeft::request_type request;
		request.set_pagination_token("invalid");

		auto *right = request.mutable_right_entity();
		right->set_id(tuple.rEntityId());
		right->set_type(tuple.rEntityType());

		rpcListLeft::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcListLeft>(ctx, request));
		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		ASSERT_TRUE(result.response);
		EXPECT_FALSE(result.response->has_pagination_token());

		auto &actual = result.response->tuples();
		ASSERT_EQ(1, actual.size());
		EXPECT_EQ(tuple.id(), actual[0].id());
	}
}
