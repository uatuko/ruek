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
		request.set_strategy(static_cast<std::uint32_t>(svc::common::strategy_t::direct));

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
		EXPECT_FALSE(actual.has_ref_id_left());
		EXPECT_FALSE(actual.has_ref_id_right());
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
		EXPECT_FALSE(actual.has_ref_id_left());
		EXPECT_FALSE(actual.has_ref_id_right());
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

	// Success: check with set strategy
	{
		// Data:
		//
		//  strand |  l_entity_id   | relation |  r_entity_id
		// --------+----------------+----------+---------------
		//         | user:jane      | member   | group:readers
		//  member | group:readers  | reader   | doc:notes.txt
		//         | user:jane      | member   | group:owners
		//  owner  | group:owners   | owner    | doc:notes.txt
		//
		// Checks:
		//   1. []user:jane/reader/doc:notes.txt - ✓
		//   2. []user:jane/owner/doc:notes.txt - ✗
		//   *. []user:jane/reader/doc:notes.txt (with cost limit of 1) - ✗

		db::Tuples tuples({
			{{
				.lEntityId   = "user:jane",
				.lEntityType = "svc_RelationsTest.Check-with_set_strategy",
				.relation    = "member",
				.rEntityId   = "group:readers",
				.rEntityType = "svc_RelationsTest.Check-with_set_strategy",
			}},
			{{
				.lEntityId   = "group:readers",
				.lEntityType = "svc_RelationsTest.Check-with_set_strategy",
				.relation    = "reader",
				.rEntityId   = "doc:notes.txt",
				.rEntityType = "svc_RelationsTest.Check-with_set_strategy",
				.strand      = "member",
			}},
			{{
				.lEntityId   = "user:jane",
				.lEntityType = "svc_RelationsTest.Check-with_set_strategy",
				.relation    = "member",
				.rEntityId   = "group:owners",
				.rEntityType = "svc_RelationsTest.Check-with_set_strategy",
			}},
			{{
				.lEntityId   = "group:owners",
				.lEntityType = "svc_RelationsTest.Check-with_set_strategy",
				.relation    = "owner",
				.rEntityId   = "doc:notes.txt",
				.rEntityType = "svc_RelationsTest.Check-with_set_strategy",
				.strand      = "owner",
			}},
		});

		for (auto &t : tuples) {
			ASSERT_NO_THROW(t.store());
		}

		rpcCheck::request_type request;
		request.set_strategy(static_cast<std::uint32_t>(svc::common::strategy_t::set));

		rpcCheck::result_type result;

		// Check 1 - []user:jane/reader/doc:notes.txt
		{
			auto *left = request.mutable_left_entity();
			left->set_id(tuples[0].lEntityId());
			left->set_type(tuples[0].lEntityType());

			request.set_relation(tuples[1].relation());

			auto *right = request.mutable_right_entity();
			right->set_id(tuples[1].rEntityId());
			right->set_type(tuples[1].rEntityType());

			EXPECT_NO_THROW(result = svc.call<rpcCheck>(ctx, request));

			EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
			ASSERT_TRUE(result.response);
			EXPECT_EQ(true, result.response->found());
			EXPECT_EQ(2, result.response->cost());
			ASSERT_TRUE(result.response->has_tuple());

			auto &actual = result.response->tuple();
			EXPECT_EQ(tuples[0].spaceId(), actual.space_id());
			EXPECT_TRUE(actual.id().empty());
			EXPECT_FALSE(actual.has_left_principal_id());
			EXPECT_EQ(tuples[0].lEntityId(), actual.left_entity().id());
			EXPECT_EQ(tuples[0].lEntityType(), actual.left_entity().type());
			EXPECT_EQ(tuples[1].relation(), actual.relation());
			EXPECT_FALSE(actual.has_right_principal_id());
			EXPECT_EQ(tuples[1].rEntityId(), actual.right_entity().id());
			EXPECT_EQ(tuples[1].rEntityType(), actual.right_entity().type());
			EXPECT_TRUE(actual.strand().empty());
			EXPECT_FALSE(actual.has_attrs());
			EXPECT_EQ(tuples[0].id(), actual.ref_id_left());
			EXPECT_EQ(tuples[1].id(), actual.ref_id_right());
		}

		// Check 2 - []user:jane/owner/doc:notes.txt
		{
			auto *left = request.mutable_left_entity();
			left->set_id(tuples[0].lEntityId());
			left->set_type(tuples[0].lEntityType());

			request.set_relation(tuples[3].relation());

			auto *right = request.mutable_right_entity();
			right->set_id(tuples[3].rEntityId());
			right->set_type(tuples[3].rEntityType());

			EXPECT_NO_THROW(result = svc.call<rpcCheck>(ctx, request));

			EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
			ASSERT_TRUE(result.response);
			EXPECT_FALSE(result.response->found());
			EXPECT_EQ(3, result.response->cost());
			EXPECT_FALSE(result.response->has_tuple());
		}

		// Check * - []user:jane/reader/doc:notes.txt (with cost limit of 1)
		// This must be the last check to ensure it doesn't impact other tests.
		{
			request.set_cost_limit(1);

			auto *left = request.mutable_left_entity();
			left->set_id(tuples[0].lEntityId());
			left->set_type(tuples[0].lEntityType());

			request.set_relation(tuples[1].relation());

			auto *right = request.mutable_right_entity();
			right->set_id(tuples[1].rEntityId());
			right->set_type(tuples[1].rEntityType());

			EXPECT_NO_THROW(result = svc.call<rpcCheck>(ctx, request));

			EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
			ASSERT_TRUE(result.response);
			EXPECT_FALSE(result.response->found());
			EXPECT_EQ(-1, result.response->cost());
			EXPECT_FALSE(result.response->has_tuple());
		}
	}

	// Success: check with graph strategy
	{
		// Data:
		//
		//  strand |  l_entity_id   | relation |  r_entity_id
		// --------+----------------+----------+---------------
		//         | user:jane      | member   | group:admins
		//  member | group:admins   | member   | group:writers
		//  member | group:writers  | member   | group:readers
		//  member | group:readers  | reader   | doc:notes.txt
		//  member | group:readers  | member   | group:loop
		//  member | group:loop     | reader   | doc:notes.txt
		//  owner  | group:writers  | owner    | doc:notes.txt
		//
		// Checks:
		//   1. []user:jane/reader/doc:notes.txt - ✓
		//   2. []user:jane/owner/doc:notes.txt - ✗
		//   *. []user:jane/member/group:loop (with cost limit of 2) - ✗

		db::Tuples tuples({
			{{
				.lEntityId   = "user:jane",
				.lEntityType = "svc_RelationsTest.Check-with_graph_strategy",
				.relation    = "member",
				.rEntityId   = "group:admins",
				.rEntityType = "svc_RelationsTest.Check-with_graph_strategy",
			}},
			{{
				.lEntityId   = "group:admins",
				.lEntityType = "svc_RelationsTest.Check-with_graph_strategy",
				.relation    = "member",
				.rEntityId   = "group:writers",
				.rEntityType = "svc_RelationsTest.Check-with_graph_strategy",
				.strand      = "member",
			}},
			{{
				.lEntityId   = "group:writers",
				.lEntityType = "svc_RelationsTest.Check-with_graph_strategy",
				.relation    = "member",
				.rEntityId   = "group:readers",
				.rEntityType = "svc_RelationsTest.Check-with_graph_strategy",
				.strand      = "member",
			}},
			{{
				.lEntityId   = "group:readers",
				.lEntityType = "svc_RelationsTest.Check-with_graph_strategy",
				.relation    = "reader",
				.rEntityId   = "doc:notes.txt",
				.rEntityType = "svc_RelationsTest.Check-with_graph_strategy",
				.strand      = "member",
			}},
			{{
				.lEntityId   = "group:readers",
				.lEntityType = "svc_RelationsTest.Check-with_graph_strategy",
				.relation    = "member",
				.rEntityId   = "group:loop",
				.rEntityType = "svc_RelationsTest.Check-with_graph_strategy",
				.strand      = "member",
			}},
			{{
				.lEntityId   = "group:loop",
				.lEntityType = "svc_RelationsTest.Check-with_graph_strategy",
				.relation    = "reader",
				.rEntityId   = "doc:notes.txt",
				.rEntityType = "svc_RelationsTest.Check-with_graph_strategy",
				.strand      = "member",
			}},
			{{
				.lEntityId   = "group:writers",
				.lEntityType = "svc_RelationsTest.Check-with_graph_strategy",
				.relation    = "owner",
				.rEntityId   = "doc:notes.txt",
				.rEntityType = "svc_RelationsTest.Check-with_graph_strategy",
				.strand      = "owner",
			}},
		});

		for (auto &t : tuples) {
			ASSERT_NO_THROW(t.store());
		}

		rpcCheck::request_type request;
		request.set_strategy(static_cast<std::uint32_t>(svc::common::strategy_t::graph));

		rpcCheck::result_type result;

		// Check 1 - []user:jane/reader/doc:notes.txt
		{
			auto *left = request.mutable_left_entity();
			left->set_id(tuples[0].lEntityId());
			left->set_type(tuples[0].lEntityType());

			request.set_relation(tuples[3].relation());

			auto *right = request.mutable_right_entity();
			right->set_id(tuples[3].rEntityId());
			right->set_type(tuples[3].rEntityType());

			EXPECT_NO_THROW(result = svc.call<rpcCheck>(ctx, request));

			EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
			ASSERT_TRUE(result.response);
			EXPECT_EQ(true, result.response->found());
			EXPECT_EQ(6, result.response->cost());
			EXPECT_FALSE(result.response->has_tuple());
			ASSERT_EQ(4, result.response->path().size());

			const auto &actual = result.response->path();
			EXPECT_EQ(tuples[0].id(), actual[0].id());
			EXPECT_EQ(tuples[1].id(), actual[1].id());
			EXPECT_EQ(tuples[2].id(), actual[2].id());
			EXPECT_EQ(tuples[3].id(), actual[3].id());
		}

		// Check 2 - []user:jane/owner/doc:notes.txt
		{
			auto *left = request.mutable_left_entity();
			left->set_id(tuples[0].lEntityId());
			left->set_type(tuples[0].lEntityType());

			request.set_relation("woner");

			auto *right = request.mutable_right_entity();
			right->set_id(tuples[3].rEntityId());
			right->set_type(tuples[3].rEntityType());

			EXPECT_NO_THROW(result = svc.call<rpcCheck>(ctx, request));

			EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
			ASSERT_TRUE(result.response);
			EXPECT_EQ(false, result.response->found());
			EXPECT_EQ(1, result.response->cost());
			EXPECT_FALSE(result.response->has_tuple());
			EXPECT_TRUE(result.response->path().empty());
		}

		// Check * - []user:jane/reader/doc:notes.txt (with cost limit of 2)
		// This must be the last check to ensure it doesn't impact other tests.
		{
			request.set_cost_limit(2);

			auto *left = request.mutable_left_entity();
			left->set_id(tuples[0].lEntityId());
			left->set_type(tuples[0].lEntityType());

			request.set_relation(tuples[3].relation());

			auto *right = request.mutable_right_entity();
			right->set_id(tuples[3].rEntityId());
			right->set_type(tuples[3].rEntityType());

			EXPECT_NO_THROW(result = svc.call<rpcCheck>(ctx, request));

			EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
			ASSERT_TRUE(result.response);
			EXPECT_FALSE(result.response->found());
			EXPECT_EQ(-4, result.response->cost());
			EXPECT_FALSE(result.response->has_tuple());
			EXPECT_TRUE(result.response->path().empty());
		}
	}

	// Error: invalid strategy
	{
		rpcCheck::request_type request;
		request.set_strategy(0);

		rpcCheck::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcCheck>(ctx, request));

		EXPECT_EQ(grpcxx::status::code_t::invalid_argument, result.status.code());
		EXPECT_EQ(
			"CAMSLltzZW50aXVtOjIuMi4xLjQwMF0gSW52YWxpZCByZWxhdGlvbnMgc3RyYXRlZ3k=",
			result.status.details());

		EXPECT_FALSE(result.response);
	}
}

TEST_F(svc_RelationsTest, Create) {
	grpcxx::context ctx;
	svc::Relations  svc;

	// Success: create relation
	{
		rpcCreate::request_type request;
		request.set_optimize(static_cast<std::uint32_t>(svc::common::strategy_t::graph));

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
		EXPECT_TRUE(result.response->computed_tuples().empty());

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

		EXPECT_FALSE(actual.has_ref_id_left());
		EXPECT_FALSE(actual.has_ref_id_right());
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
		EXPECT_TRUE(result.response->computed_tuples().empty());

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
		EXPECT_FALSE(actual.has_ref_id_left());
		EXPECT_FALSE(actual.has_ref_id_right());
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
		EXPECT_TRUE(result.response->computed_tuples().empty());

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
		EXPECT_FALSE(actual.has_ref_id_left());
		EXPECT_FALSE(actual.has_ref_id_right());
	}

	// Success: create relation with direct optimize strategy (left)
	{
		//  strand |  l_entity_id  | relation |  r_entity_id
		// --------+---------------+----------+---------------
		//         | user:jane     | member   | group:editors     <- already exists
		//  member | group:editors | parent   | group:viewers     <- create
		//         | user:jane     | parent   | group:viewers     <- compute

		db::Tuple tuple({
			.lEntityId   = "user:jane",
			.lEntityType = "svc_RelationsTest.Create-with_optimize_l",
			.relation    = "member",
			.rEntityId   = "group:editors",
			.rEntityType = "svc_RelationsTest.Create-with_optimize_l",
		});
		ASSERT_NO_THROW(tuple.store());

		rpcCreate::request_type request;
		request.set_optimize(static_cast<std::uint32_t>(svc::common::strategy_t::direct));
		request.set_strand(tuple.relation());

		auto *left = request.mutable_left_entity();
		left->set_id(tuple.rEntityId()); // group:editors
		left->set_type(tuple.rEntityType());

		request.set_relation("parent");

		auto *right = request.mutable_right_entity();
		right->set_id("group:viewers");
		right->set_type("svc_RelationsTest.Create-with_optimize_l");

		rpcCreate::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcCreate>(ctx, request));

		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		ASSERT_TRUE(result.response);
		EXPECT_EQ(2, result.response->cost());
		ASSERT_EQ(1, result.response->computed_tuples().size());

		const auto &actual = result.response->computed_tuples()[0];
		EXPECT_EQ(tuple.lEntityId(), actual.left_entity().id());
		EXPECT_EQ(request.relation(), actual.relation());
		EXPECT_EQ(request.right_entity().id(), actual.right_entity().id());
	}

	// Success: create relation with direct optimize strategy (right)
	{
		//  strand |  l_entity_id  | relation |  r_entity_id
		// --------+---------------+----------+---------------
		//         | user:john     | member   | group:writers     <- create
		//  member | group:writers | parent   | group:readers     <- already exists
		//         | user:john     | parent   | group:readers     <- compute

		db::Tuple tuple({
			.lEntityId   = "group:writers",
			.lEntityType = "svc_RelationsTest.Create-with_optimize_r",
			.relation    = "parent",
			.rEntityId   = "group:readers",
			.rEntityType = "svc_RelationsTest.Create-with_optimize_r",
			.strand      = "member",
		});
		ASSERT_NO_THROW(tuple.store());

		rpcCreate::request_type request;
		request.set_optimize(static_cast<std::uint32_t>(svc::common::strategy_t::direct));

		auto *left = request.mutable_left_entity();
		left->set_id("user:john");
		left->set_type("svc_RelationsTest.Create-with_optimize_r");

		request.set_relation("member");

		auto *right = request.mutable_right_entity();
		right->set_id(tuple.lEntityId()); // group:writers
		right->set_type(tuple.lEntityType());

		rpcCreate::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcCreate>(ctx, request));

		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		ASSERT_TRUE(result.response);
		EXPECT_EQ(2, result.response->cost());
		ASSERT_EQ(1, result.response->computed_tuples().size());

		const auto &actual = result.response->computed_tuples()[0];
		EXPECT_EQ(request.left_entity().id(), actual.left_entity().id());
		EXPECT_EQ(tuple.relation(), actual.relation());
		EXPECT_EQ(tuple.rEntityId(), actual.right_entity().id());
	}

	// Success: create relation with direct optimize strategy
	{
		//  strand  |  l_entity_id  | relation |  r_entity_id
		// ---------+---------------+----------+---------------
		//          | group:admins  | admins   | group:editors     <- already exists
		//  admin   | group:editors | editors  | group:writers     <- create
		//  editors | group:writers | readers  | group:readers     <- already exists
		//          | group:admins  | editors  | group:writers     <- compute
		//          | group:editors | readers  | group:readers     <- compute

		db::Tuples tuples({
			{{
				.lEntityId   = "group:admins",
				.lEntityType = "svc_RelationsTest.Create-with_optimize",
				.relation    = "admin",
				.rEntityId   = "group:editors",
				.rEntityType = "svc_RelationsTest.Create-with_optimize",
			}},
			{{
				.lEntityId   = "group:writers",
				.lEntityType = "svc_RelationsTest.Create-with_optimize",
				.relation    = "readers",
				.rEntityId   = "group:readers",
				.rEntityType = "svc_RelationsTest.Create-with_optimize",
				.strand      = "editors",
			}},
		});

		for (auto &t : tuples) {
			ASSERT_NO_THROW(t.store());
		}

		rpcCreate::request_type request;
		request.set_optimize(static_cast<std::uint32_t>(svc::common::strategy_t::direct));
		request.set_strand(tuples[0].relation()); // admins

		auto *left = request.mutable_left_entity();
		left->set_id(tuples[0].rEntityId()); // group:editors
		left->set_type(tuples[0].rEntityType());

		request.set_relation(tuples[1].strand()); // editors

		auto *right = request.mutable_right_entity();
		right->set_id(tuples[1].lEntityId()); // group:writers
		right->set_type(tuples[1].lEntityType());

		rpcCreate::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcCreate>(ctx, request));

		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		ASSERT_TRUE(result.response);
		EXPECT_EQ(3, result.response->cost());
		ASSERT_EQ(2, result.response->computed_tuples().size());

		const auto &actual = result.response->computed_tuples();
		// []group:admins/editors/group:writers
		EXPECT_EQ(tuples[0].lEntityId(), actual[0].left_entity().id());
		EXPECT_EQ(request.relation(), actual[0].relation());
		EXPECT_EQ(request.right_entity().id(), actual[0].right_entity().id());

		// []group:editors/readers/group:writers
		EXPECT_EQ(request.left_entity().id(), actual[1].left_entity().id());
		EXPECT_EQ(tuples[1].relation(), actual[1].relation());
		EXPECT_EQ(tuples[1].rEntityId(), actual[1].right_entity().id());
	}

	// Success: create relation with direct optimize strategy and cost limit
	{
		db::Tuple tuple({
			.lEntityId   = "group:writers",
			.lEntityType = "svc_RelationsTest.Create-with_optimize_and_cost_limit",
			.relation    = "readers",
			.rEntityId   = "group:readers",
			.rEntityType = "svc_RelationsTest.Create-with_optimize_and_cost_limit",
			.strand      = "member",
		});
		ASSERT_NO_THROW(tuple.store());

		rpcCreate::request_type request;
		request.set_optimize(static_cast<std::uint32_t>(svc::common::strategy_t::direct));
		request.set_cost_limit(1);

		auto *left = request.mutable_left_entity();
		left->set_id("user:john");
		left->set_type("svc_RelationsTest.Create-with_optimize_and_cost_limit");

		request.set_relation("member");

		auto *right = request.mutable_right_entity();
		right->set_id(tuple.lEntityId()); // group:writers
		right->set_type(tuple.lEntityType());

		rpcCreate::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcCreate>(ctx, request));

		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		ASSERT_TRUE(result.response);
		EXPECT_EQ((request.cost_limit() + 1) * -1, result.response->cost());
		ASSERT_EQ(1, result.response->computed_tuples().size());

		const auto &actual = result.response->computed_tuples()[0];
		EXPECT_TRUE(actual.id().empty());
	}

	// Success: create relation with direct optimize strategy resulting in duplicate computed entry
	{
		//  strand |  l_entity_id  | relation |  r_entity_id
		// --------+---------------+----------+---------------
		//         | user:jane     | member   | group:editors     <- already exists
		//         | user:jane     | member   | group:viewers     <- already exists
		//  member | group:editors | member   | group:viewers     <- create

		db::Tuples tuples({
			{{
				.lEntityId   = "user:jane",
				.lEntityType = "svc_RelationsTest.Create-with_optimize_duplicate",
				.relation    = "member",
				.rEntityId   = "group:editors",
				.rEntityType = "svc_RelationsTest.Create-with_optimize_duplicate",
			}},
			{{
				.lEntityId   = "user:jane",
				.lEntityType = "svc_RelationsTest.Create-with_optimize_duplicate",
				.relation    = "member",
				.rEntityId   = "group:viewers",
				.rEntityType = "svc_RelationsTest.Create-with_optimize_duplicate",
			}},
		});

		for (auto &t : tuples) {
			ASSERT_NO_THROW(t.store());
		}

		rpcCreate::request_type request;
		request.set_optimize(static_cast<std::uint32_t>(svc::common::strategy_t::direct));

		auto *left = request.mutable_left_entity();
		left->set_id(tuples[0].rEntityId()); // group:editors
		left->set_type(tuples[0].rEntityType());

		request.set_relation(tuples[1].relation());

		auto *right = request.mutable_right_entity();
		right->set_id(tuples[1].rEntityId()); // group:viewers
		right->set_type(tuples[1].rEntityType());

		request.set_strand(tuples[0].relation());

		rpcCreate::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcCreate>(ctx, request));

		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		ASSERT_TRUE(result.response);
		EXPECT_EQ(2, result.response->cost());
		EXPECT_TRUE(result.response->computed_tuples().empty());
	}

	// Success: create relations with set optimize strategy
	{
		//  strand |  l_entity_id   | relation |  r_entity_id
		// --------+----------------+----------+---------------
		//         | user:jane      | member   | group:writers     <- already exists
		//  member | group:writers  | member   | group:readers     <- create(1)
		//         | user:jane      | member   | group:readers     <- compute(1)
		//  member | group:readers  | reader   | doc:notes.txt     <- create(2)
		//  owner  | folder:home    | parent   | doc:notes.txt     <- create(3)
		//         | user:jane      | owner    | folder:home       <- create(4)
		//  parent | doc:notes.txt  | describe | group:readers     <- create(5)

		db::Principals principals({
			{{
				.attrs   = R"({"name": "jane", "type": "user"})",
				.segment = "svc_RelationsTest.Create-with_optimize_set",
			}},
			{{
				.attrs   = R"({"name": "writers", "type": "group"})",
				.segment = "svc_RelationsTest.Create-with_optimize_set",
			}},
			{{
				.attrs   = R"({"name": "readers", "type": "group"})",
				.segment = "svc_RelationsTest.Create-with_optimize_set",
			}},
		});

		for (auto &p : principals) {
			ASSERT_NO_THROW(p.store());
		}

		db::Tuple tuple({
			.lPrincipalId = principals[0].id(),
			.relation     = "member",
			.rPrincipalId = principals[1].id(),
		});
		ASSERT_NO_THROW(tuple.store());

		rpcCreate::request_type request;
		request.set_optimize(static_cast<std::uint32_t>(svc::common::strategy_t::set));

		rpcCreate::result_type result;

		// Create(1), [member]group:writers/member/group:readers
		{
			request.set_left_principal_id(principals[1].id()); // group:writers
			request.set_relation("member");
			request.set_right_principal_id(principals[2].id()); // group:readers
			request.set_strand(tuple.relation());

			EXPECT_NO_THROW(result = svc.call<rpcCreate>(ctx, request));

			EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
			ASSERT_TRUE(result.response);
			EXPECT_EQ(2, result.response->cost());
			ASSERT_EQ(1, result.response->computed_tuples().size());

			const auto &actual = result.response->computed_tuples();
			// []user:jane/member/group:readers
			EXPECT_EQ(principals[0].id(), actual[0].left_principal_id());
			EXPECT_EQ(request.relation(), actual[0].relation());
			EXPECT_EQ(principals[2].id(), actual[0].right_principal_id());
		}

		// Create(2), [member]group:readers/reader/doc:notes.txt
		{
			request.set_left_principal_id(principals[2].id()); // group:readers
			request.set_relation("reader");

			auto *right = request.mutable_right_entity();
			right->set_id("doc:notes.txt");
			right->set_type("svc_RelationsTest.Create-with_optimize_set");

			request.set_strand(tuple.relation());

			EXPECT_NO_THROW(result = svc.call<rpcCreate>(ctx, request));

			EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
			ASSERT_TRUE(result.response);
			EXPECT_EQ(1, result.response->cost());
			EXPECT_TRUE(result.response->computed_tuples().empty());
		}

		// Create(3), [owner]folder:home/parent/doc:notes.txt
		{
			auto *left = request.mutable_left_entity();
			left->set_id("folder:home");
			left->set_type("svc_RelationsTest.Create-with_optimize_set");

			request.set_relation("parent");

			auto *right = request.mutable_right_entity();
			right->set_id("doc:notes.txt");
			right->set_type("svc_RelationsTest.Create-with_optimize_set");

			request.set_strand("owner");

			EXPECT_NO_THROW(result = svc.call<rpcCreate>(ctx, request));

			EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
			ASSERT_TRUE(result.response);
			EXPECT_EQ(1, result.response->cost());
			EXPECT_TRUE(result.response->computed_tuples().empty());
		}

		// Create(4), []user:jane/owner/folder:home
		{
			request.set_left_principal_id(principals[0].id()); // user:jane
			request.set_relation("owner");

			auto *right = request.mutable_right_entity();
			right->set_id("folder:home");
			right->set_type("svc_RelationsTest.Create-with_optimize_set");

			request.clear_strand();

			EXPECT_NO_THROW(result = svc.call<rpcCreate>(ctx, request));

			EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
			ASSERT_TRUE(result.response);
			EXPECT_EQ(2, result.response->cost());
			EXPECT_TRUE(result.response->computed_tuples().empty());
		}

		// Create(5), [parent]doc:notes.txt/describe/group:readers
		{
			auto *left = request.mutable_left_entity();
			left->set_id("doc:notes.txt");
			left->set_type("svc_RelationsTest.Create-with_optimize_set");

			request.set_relation("describe");
			request.set_right_principal_id(principals[2].id()); // group:readers
			request.set_strand("parent");

			EXPECT_NO_THROW(result = svc.call<rpcCreate>(ctx, request));

			EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
			ASSERT_TRUE(result.response);
			EXPECT_EQ(2, result.response->cost());
			EXPECT_TRUE(result.response->computed_tuples().empty());
		}
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

	// Error: invalid optmization strategy
	{
		rpcCreate::request_type request;
		request.set_optimize(0);

		rpcCreate::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcCreate>(ctx, request));

		EXPECT_EQ(grpcxx::status::code_t::invalid_argument, result.status.code());
		EXPECT_EQ(
			"CAMSLltzZW50aXVtOjIuMi4xLjQwMF0gSW52YWxpZCByZWxhdGlvbnMgc3RyYXRlZ3k=",
			result.status.details());

		EXPECT_FALSE(result.response);
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
		db::Principal principal({.id = "id:svc_RelationsTest.list-left"});
		ASSERT_NO_THROW(principal.store());

		db::Tuple tuple({
			.lEntityId    = "left",
			.lEntityType  = "svc_RelationsTest.list",
			.relation     = "relation",
			.rPrincipalId = principal.id(),
		});
		ASSERT_NO_THROW(tuple.store());

		rpcListLeft::request_type request;
		request.set_right_principal_id(principal.id());

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
		EXPECT_TRUE(actual[0].has_right_principal_id());
		EXPECT_EQ(principal.id(), actual[0].right_principal_id());
		EXPECT_FALSE(actual[0].has_strand());
		EXPECT_FALSE(actual[0].has_attrs());
		EXPECT_FALSE(actual[0].has_ref_id_left());
		EXPECT_FALSE(actual[0].has_ref_id_right());
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

	// Success: list left with invalid pagination token
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

TEST_F(svc_RelationsTest, ListRight) {
	grpcxx::context ctx;
	svc::Relations  svc;

	// Success: list right
	{
		db::Principal principal({.id = "id:svc_RelationsTest.list-right"});
		ASSERT_NO_THROW(principal.store());

		db::Tuple tuple({
			.lPrincipalId = principal.id(),
			.relation     = "relation",
			.rEntityId    = "right",
			.rEntityType  = "svc_RelationsTest.list",
		});
		ASSERT_NO_THROW(tuple.store());

		rpcListRight::request_type request;
		request.set_left_principal_id(principal.id());

		rpcListRight::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcListRight>(ctx, request));
		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		ASSERT_TRUE(result.response);

		EXPECT_FALSE(result.response->has_pagination_token());

		auto &actual = result.response->tuples();
		ASSERT_EQ(1, actual.size());
		EXPECT_EQ(tuple.spaceId(), actual[0].space_id());
		EXPECT_EQ(tuple.id(), actual[0].id());
		EXPECT_TRUE(actual[0].has_left_principal_id());
		EXPECT_EQ(principal.id(), actual[0].left_principal_id());
		EXPECT_EQ(tuple.relation(), actual[0].relation());
		EXPECT_FALSE(actual[0].has_right_principal_id());
		EXPECT_EQ(tuple.rEntityId(), actual[0].right_entity().id());
		EXPECT_EQ(tuple.rEntityType(), actual[0].right_entity().type());
		EXPECT_FALSE(actual[0].has_strand());
		EXPECT_FALSE(actual[0].has_attrs());
		EXPECT_FALSE(actual[0].has_ref_id_left());
		EXPECT_FALSE(actual[0].has_ref_id_right());
	}

	// Success: list right with relation
	{
		db::Tuples tuples({
			{{
				.lEntityId   = "left",
				.lEntityType = "svc_RelationsTest.list-right_with_relation",
				.relation    = "relation[0]",
				.rEntityId   = "right",
				.rEntityType = "svc_RelationsTest.list",
			}},
			{{
				.lEntityId   = "left",
				.lEntityType = "svc_RelationsTest.list-right_with_relation",
				.relation    = "relation[1]",
				.rEntityId   = "right",
				.rEntityType = "svc_RelationsTest.list",
			}},
		});

		for (auto &t : tuples) {
			ASSERT_NO_THROW(t.store());
		}

		rpcListRight::request_type request;

		auto *left = request.mutable_left_entity();
		left->set_id(tuples[0].lEntityId());
		left->set_type(tuples[0].lEntityType());

		request.set_relation(tuples[0].relation());

		rpcListRight::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcListRight>(ctx, request));
		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		ASSERT_TRUE(result.response);
		EXPECT_FALSE(result.response->has_pagination_token());

		auto &actual = result.response->tuples();
		ASSERT_EQ(1, actual.size());
		EXPECT_EQ(tuples[0].id(), actual[0].id());
	}

	// Success: list right with space-id
	{
		db::Tuples tuples({
			{{
				.lEntityId   = "left",
				.lEntityType = "svc_RelationsTest.list-right_with_space_id",
				.relation    = "relation",
				.rEntityId   = "right",
				.rEntityType = "svc_RelationsTest.list",
				.spaceId     = "space_id[0]",
			}},
			{{
				.lEntityId   = "left",
				.lEntityType = "svc_RelationsTest.list-right_with_space_id",
				.relation    = "relation",
				.rEntityId   = "right",
				.rEntityType = "svc_RelationsTest.list",
				.spaceId     = "space_id[1]",
			}},
		});

		for (auto &t : tuples) {
			ASSERT_NO_THROW(t.store());
		}

		grpcxx::detail::request r(1);
		r.header(std::string(svc::common::space_id_v), std::string(tuples[1].spaceId()));

		grpcxx::context ctx(r);

		rpcListRight::request_type request;

		auto *left = request.mutable_left_entity();
		left->set_id(tuples[1].lEntityId());
		left->set_type(tuples[1].lEntityType());

		rpcListRight::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcListRight>(ctx, request));
		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		ASSERT_TRUE(result.response);
		EXPECT_FALSE(result.response->has_pagination_token());

		auto &actual = result.response->tuples();
		ASSERT_EQ(1, actual.size());
		EXPECT_EQ(tuples[1].id(), actual[0].id());
	}

	// Success: list right with pagination
	{
		db::Tuples tuples({
			{{
				.lEntityId   = "left",
				.lEntityType = "svc_RelationsTest.list-right_with_pagination",
				.relation    = "relation",
				.rEntityId   = "right[0]",
				.rEntityType = "svc_RelationsTest.list",
			}},
			{{
				.lEntityId   = "left",
				.lEntityType = "svc_RelationsTest.list-right_with_pagination",
				.relation    = "relation",
				.rEntityId   = "right[1]",
				.rEntityType = "svc_RelationsTest.list",
			}},
		});

		for (auto &t : tuples) {
			ASSERT_NO_THROW(t.store());
		}

		rpcListRight::request_type request;
		request.set_pagination_limit(1);

		auto *left = request.mutable_left_entity();
		left->set_id(tuples[1].lEntityId());
		left->set_type(tuples[1].lEntityType());

		rpcListRight::result_type result;

		// Page 1
		{
			EXPECT_NO_THROW(result = svc.call<rpcListRight>(ctx, request));
			EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
			ASSERT_TRUE(result.response);

			ASSERT_TRUE(result.response->has_pagination_token());
			EXPECT_EQ("18474qb7d1q5mcat", result.response->pagination_token());

			auto &actual = result.response->tuples();
			ASSERT_EQ(1, actual.size());
			EXPECT_EQ(tuples[1].id(), actual[0].id());
		}

		// Use pagination token to get the next page of results
		request.set_pagination_token(result.response->pagination_token());

		// Page 2
		{
			EXPECT_NO_THROW(result = svc.call<rpcListRight>(ctx, request));
			EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
			ASSERT_TRUE(result.response);

			ASSERT_TRUE(result.response->has_pagination_token());
			EXPECT_EQ("18474qb7d1q5mc2t", result.response->pagination_token());

			auto &actual = result.response->tuples();
			ASSERT_EQ(1, actual.size());
			EXPECT_EQ(tuples[0].id(), actual[0].id());
		}
	}

	// Success: list right with invalid pagination token
	{
		db::Tuple tuple({
			.lEntityId   = "left",
			.lEntityType = "svc_RelationsTest.list-right_with_invalid_pagination_token",
			.relation    = "relation",
			.rEntityId   = "right",
			.rEntityType = "svc_RelationsTest.list",
		});
		ASSERT_NO_THROW(tuple.store());

		rpcListRight::request_type request;
		request.set_pagination_token("invalid");

		auto *left = request.mutable_left_entity();
		left->set_id(tuple.lEntityId());
		left->set_type(tuple.lEntityType());

		rpcListRight::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcListRight>(ctx, request));
		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		ASSERT_TRUE(result.response);
		EXPECT_FALSE(result.response->has_pagination_token());

		auto &actual = result.response->tuples();
		ASSERT_EQ(1, actual.size());
		EXPECT_EQ(tuple.id(), actual[0].id());
	}
}
