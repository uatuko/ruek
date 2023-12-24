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

	// Success: create principal with `segment`
	{
		rpcCreate::request_type request;
		request.set_segment("segment:svc_PrincipalsTest.Create-with_segment");

		rpcCreate::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcCreate>(ctx, request));

		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		ASSERT_TRUE(result.response);
		EXPECT_EQ("segment:svc_PrincipalsTest.Create-with_segment", result.response->segment());
	}

	// Error: invalid `segment`
	{
		rpcCreate::request_type request;
		request.set_segment("");

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

TEST_F(svc_PrincipalsTest, Delete) {
	grpcxx::context ctx;
	svc::Principals svc;

	// Success: delete
	{
		db::Principal principal({
			.id = "id:svc_PrincipalsTest-Delete",
		});
		ASSERT_NO_THROW(principal.store());

		rpcDelete::request_type request;
		request.set_id(principal.id());

		rpcDelete::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcDelete>(ctx, request));
		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		EXPECT_TRUE(result.response);
	}

	// Error: not found
	{
		rpcDelete::request_type request;
		request.set_id("id:svc_PrincipalsTest-Delete-non_existent");

		rpcDelete::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcDelete>(ctx, request));
		EXPECT_EQ(grpcxx::status::code_t::not_found, result.status.code());
		EXPECT_FALSE(result.response);
	}
}

TEST_F(svc_PrincipalsTest, Retrieve) {
	grpcxx::context ctx;
	svc::Principals svc;

	// Success: retrieve
	{
		db::Principal principal({
			.id = "id:svc_PrincipalsTest-Retrieve",
		});
		ASSERT_NO_THROW(principal.store());

		rpcRetrieve::request_type request;
		request.set_id(principal.id());

		rpcRetrieve::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcRetrieve>(ctx, request));
		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		EXPECT_TRUE(result.response);

		auto &actual = result.response.value();
		EXPECT_EQ(principal.id(), actual.id());
		EXPECT_FALSE(actual.has_attrs());
		EXPECT_FALSE(actual.has_segment());
	}

	// Success: retrieve with `attrs`
	{
		db::Principal principal({
			.attrs = R"({"foo":"bar"})",
			.id    = "id:svc_PrincipalsTest-Retrieve-with_attrs",
		});
		ASSERT_NO_THROW(principal.store());

		rpcRetrieve::request_type request;
		request.set_id(principal.id());

		rpcRetrieve::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcRetrieve>(ctx, request));
		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		EXPECT_TRUE(result.response);

		auto &actual = result.response.value();
		EXPECT_EQ(principal.id(), actual.id());
		EXPECT_TRUE(actual.has_attrs());
		EXPECT_FALSE(actual.has_segment());

		std::string responseAttrs;
		google::protobuf::util::MessageToJsonString(result.response->attrs(), &responseAttrs);
		EXPECT_EQ(principal.attrs(), responseAttrs);
	}

	// Error: not found
	{
		rpcRetrieve::request_type request;
		request.set_id("id:svc_PrincipalsTest-Retrieve-non_existent");

		rpcRetrieve::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcRetrieve>(ctx, request));
		EXPECT_EQ(grpcxx::status::code_t::not_found, result.status.code());
		EXPECT_FALSE(result.response);
	}
}

TEST_F(svc_PrincipalsTest, Update) {
	grpcxx::context ctx;
	svc::Principals svc;

	// Success: update
	{
		db::Principal principal({
			.id = "id:svc_PrincipalsTest-Update",
		});
		ASSERT_NO_THROW(principal.store());

		rpcUpdate::request_type request;
		request.set_id(principal.id());
		request.set_segment("segment:svc_PrincipalsTest-Update");

		const std::string attrs(R"({"foo":"bar"})");
		google::protobuf::util::JsonStringToMessage(attrs, request.mutable_attrs());

		rpcUpdate::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcUpdate>(ctx, request));
		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		EXPECT_TRUE(result.response);

		auto &actual = result.response.value();
		EXPECT_EQ(principal.id(), actual.id());
		EXPECT_EQ("segment:svc_PrincipalsTest-Update", actual.segment());
		EXPECT_TRUE(actual.has_attrs());

		std::string responseAttrs;
		google::protobuf::util::MessageToJsonString(result.response->attrs(), &responseAttrs);
		EXPECT_EQ(attrs, responseAttrs);
	}

	// Success: short-circuit where nothing to update
	{
		db::Principal principal({
			.id = "id:svc_PrincipalsTest-Update-short_circuit",
		});
		ASSERT_NO_THROW(principal.store());

		rpcUpdate::request_type request;
		request.set_id(principal.id());

		rpcUpdate::result_type result;
		EXPECT_NO_THROW(result = svc.call<rpcUpdate>(ctx, request));
		EXPECT_EQ(grpcxx::status::code_t::ok, result.status.code());
		EXPECT_TRUE(result.response);

		auto &actual = result.response.value();
		EXPECT_EQ(principal.id(), actual.id());
		EXPECT_FALSE(actual.has_segment());
		EXPECT_FALSE(actual.has_attrs());

		auto p = db::Principal::retrieve(principal.id());
		EXPECT_EQ(principal.rev(), p.rev());
	}
}
