#include "gatekeeper.h"

#include <grpcpp/test/default_reactor_test_peer.h>
#include <gtest/gtest.h>

#include "datastore/identities.h"
#include "datastore/testing.h"

class GatekeeperIdentitiesTest : public testing::Test {
protected:
	static void SetUpTestSuite() {
		datastore::testing::setup();

		// Clear data
		datastore::pg::exec("truncate table identities cascade;");
	}

	static void TearDownTestSuite() { datastore::testing::teardown(); }
};

TEST_F(GatekeeperIdentitiesTest, CreateIdentity) {
	service::Gatekeeper service;

	// Success: create identity
	{
		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Identity                      response;

		gk::v1::CreateIdentityRequest request;
		request.set_sub("sub:GatekeeperTest.CreateIdentity");

		auto reactor = service.CreateIdentity(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);
		EXPECT_EQ(request.sub(), response.sub());
		EXPECT_FALSE(response.id().empty());
		EXPECT_FALSE(response.has_attrs());
	}

	// Success: create identity with `id`
	{
		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Identity                      response;

		gk::v1::CreateIdentityRequest request;
		request.set_id("id:GatekeeperTest.CreateIdentity-with_id");
		request.set_sub("sub:GatekeeperTest.CreateIdentity-with_id");

		auto reactor = service.CreateIdentity(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);
		EXPECT_EQ(request.id(), response.id());
		EXPECT_EQ(request.sub(), response.sub());
		EXPECT_FALSE(response.has_attrs());
	}

	// Success: create identity with `attrs`
	{
		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Identity                      response;

		gk::v1::CreateIdentityRequest request;
		request.set_sub("sub:GatekeeperTest.CreateIdentity-with_attrs");

		const std::string attrs(R"({"foo":"bar"})");
		google::protobuf::util::JsonStringToMessage(attrs, request.mutable_attrs());

		auto reactor = service.CreateIdentity(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);
		EXPECT_EQ(request.sub(), response.sub());
		EXPECT_TRUE(response.has_attrs());

		std::string responseAttrs;
		google::protobuf::util::MessageToJsonString(response.attrs(), &responseAttrs);
		EXPECT_EQ(attrs, responseAttrs);
	}

	// Error: duplicate `id`
	{
		const datastore::Identity identity({
			.sub = "sub:GatekeeperTest.CreateIdentity-duplicate_id",
		});

		try {
			identity.store();
		} catch (const std::exception &e) {
			FAIL() << e.what();
		}

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Identity                      response;

		gk::v1::CreateIdentityRequest request;
		request.set_id(identity.id());
		request.set_sub("sub:GatekeeperTest.CreateIdentity-duplicate_id");

		auto reactor = service.CreateIdentity(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_EQ(grpc::StatusCode::ALREADY_EXISTS, peer.test_status().error_code());
		EXPECT_EQ("Duplicate identity id", peer.test_status().error_message());
	}

	// Error: duplicate `sub`
	{
		const datastore::Identity identity({
			.sub = "sub:GatekeeperTest.CreateIdentity-duplicate",
		});
		EXPECT_NO_THROW(identity.store());

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Identity                      response;

		gk::v1::CreateIdentityRequest request;
		request.set_sub(identity.sub());

		auto reactor = service.CreateIdentity(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_EQ(grpc::StatusCode::ALREADY_EXISTS, peer.test_status().error_code());
		EXPECT_EQ("Duplicate identity", peer.test_status().error_message());
	}
}

TEST_F(GatekeeperIdentitiesTest, RetrieveIdentity) {
	service::Gatekeeper service;

	// Success: retrieve identity by id
	{
		const datastore::Identity identity({
			.id  = "id:GatekeeperTest.RetrieveIdentity",
			.sub = "sub:GatekeeperTest.RetrieveIdentity",
		});
		ASSERT_NO_THROW(identity.store());

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Identity                      response;

		gk::v1::RetrieveIdentityRequest request;
		request.set_id(identity.id());

		auto reactor = service.RetrieveIdentity(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);
		EXPECT_EQ(identity.id(), response.id());
		EXPECT_EQ(identity.sub(), response.sub());
		EXPECT_FALSE(response.has_attrs());
	}

	// Success: retrieve identity by sub
	{
		const datastore::Identity identity({
			.id  = "id:GatekeeperTest.RetrieveIdentity-by_sub",
			.sub = "sub:GatekeeperTest.RetrieveIdentity-by_sub",
		});
		ASSERT_NO_THROW(identity.store());

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Identity                      response;

		gk::v1::RetrieveIdentityRequest request;
		request.set_sub(identity.sub());

		auto reactor = service.RetrieveIdentity(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);
		EXPECT_EQ(identity.id(), response.id());
		EXPECT_EQ(identity.sub(), response.sub());
		EXPECT_FALSE(response.has_attrs());
	}

	// Success: retrieve identity by id with attrs
	{
		const datastore::Identity identity({
			.attrs = R"({"flag":true})",
			.sub   = "sub:GatekeeperTest.RetrieveIdentity-with_attrs",
		});
		ASSERT_NO_THROW(identity.store());

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Identity                      response;

		gk::v1::RetrieveIdentityRequest request;
		request.set_id(identity.id());

		auto reactor = service.RetrieveIdentity(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);
		EXPECT_EQ(identity.id(), response.id());
		EXPECT_EQ(identity.sub(), response.sub());
		EXPECT_TRUE(response.has_attrs());

		std::string attrs;
		google::protobuf::util::MessageToJsonString(response.attrs(), &attrs);
		EXPECT_EQ(*identity.attrs(), attrs);
	}

	// Error: identity not found
	{
		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Identity                      response;

		gk::v1::RetrieveIdentityRequest request;
		request.set_id("id:GatekeeperTest.RetrieveIdentity-not_found");

		auto reactor = service.RetrieveIdentity(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_EQ(grpc::StatusCode::NOT_FOUND, peer.test_status().error_code());
		EXPECT_EQ("Document not found", peer.test_status().error_message());
	}
}

TEST_F(GatekeeperIdentitiesTest, UpdateIndentity) {
	service::Gatekeeper service;

	// Success: update identity sub
	{
		const datastore::Identity identity(
			{.id  = "id:GatekeeperTest.UpdateIdentity-sub",
			 .sub = "sub:GatekeeperTest.UpdateIdentity-sub"});
		ASSERT_NO_THROW(identity.store());

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Identity                      response;

		gk::v1::UpdateIdentityRequest request;
		request.set_id(identity.id());
		request.set_sub("sub:GatekeeperTest.UpdateIdentity-new");

		auto reactor = service.UpdateIdentity(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);

		EXPECT_EQ(request.id(), response.id());
		EXPECT_EQ(request.sub(), response.sub());
		EXPECT_FALSE(response.has_attrs());

		auto updatedIdentity = datastore::RetrieveIdentity(identity.id());
		EXPECT_EQ(request.sub(), updatedIdentity.sub());
		EXPECT_EQ(1, updatedIdentity.rev());
		EXPECT_FALSE(updatedIdentity.attrs());
	}

	// Success: update identity attrs
	{
		const datastore::Identity identity({.sub = "sub:GatekeeperTest.UpdateIdentity-attrs"});
		ASSERT_NO_THROW(identity.store());

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Identity                      response;

		gk::v1::UpdateIdentityRequest request;
		request.set_id(identity.id());
		{
			auto &attrs  = *request.mutable_attrs();
			auto &fields = *attrs.mutable_fields();

			{
				google::protobuf::Value v;
				v.set_bool_value(true);

				fields["flag"] = v;
			}

			{
				google::protobuf::Value v;
				v.set_string_value("Jane Doe");

				fields["name"] = v;
			}
		}

		auto reactor = service.UpdateIdentity(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);

		ASSERT_TRUE(response.has_attrs());
		auto &req = *request.mutable_attrs()->mutable_fields();
		auto &res = *response.mutable_attrs()->mutable_fields();

		EXPECT_EQ(req["flag"].bool_value(), res["flag"].bool_value());
		EXPECT_EQ(req["name"].string_value(), res["name"].string_value());

		{
			std::string_view qry = R"(
					select
						attrs->'flag' as flag,
						attrs->>'name' as name
					from identities
					where _id = $1::text;
				)";

			auto res = datastore::pg::exec(qry, identity.id());
			ASSERT_EQ(1, res.size());

			auto [flag, name] = res[0].as<bool, std::string>();
			EXPECT_EQ(req["flag"].bool_value(), flag);
			EXPECT_EQ(req["name"].string_value(), name);
		}
	}

	// Error: no fields to update
	{
		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Identity                      response;

		gk::v1::UpdateIdentityRequest request;
		request.set_id("id:GatekeeperTest.UpdateIdentity-no-updates");

		auto reactor = service.UpdateIdentity(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_EQ(grpc::StatusCode::INTERNAL, peer.test_status().error_code());
		EXPECT_EQ("No fields to update", peer.test_status().error_message());
	}

	// Error: identity not found
	{
		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Identity                      response;

		gk::v1::UpdateIdentityRequest request;
		request.set_id("id:GatekeeperTest.UpdateIdentity-not-found");
		request.set_sub("name:GatekeeperTest.UpdateIdentity-not-found");

		auto reactor = service.UpdateIdentity(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_EQ(grpc::StatusCode::NOT_FOUND, peer.test_status().error_code());
		EXPECT_EQ("Document not found", peer.test_status().error_message());
	}
}
