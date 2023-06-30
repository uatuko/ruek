#include <grpcpp/test/default_reactor_test_peer.h>
#include <gtest/gtest.h>

#include "datastore/collections.h"
#include "datastore/identities.h"
#include "datastore/testing.h"

#include "grpc.h"

class GrpcTest : public ::testing::Test {
protected:
	static void SetUpTestSuite() {
		datastore::testing::setup();

		// Clear data
		datastore::pg::exec("truncate table collections cascade;");
		datastore::pg::exec("truncate table identities cascade;");
	}

	void SetUp() {
		// Clear data before each test
		datastore::pg::exec("delete from collections cascade;");
		datastore::pg::exec("delete from identities cascade;");
	}

	static void TearDownTestSuite() { datastore::testing::teardown(); }
};

// Collections
TEST_F(GrpcTest, CreateCollection) {
	service::Grpc service;

	// Success: create collection
	{
		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Collection                    response;

		gk::v1::CreateCollectionRequest request;
		request.set_name("name:GrpcTest.CreateCollection");

		auto reactor = service.CreateCollection(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);
		EXPECT_EQ(request.name(), response.name());
		EXPECT_FALSE(response.id().empty());
	}

	// Success: create collection with `id`
	{
		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Collection                    response;

		gk::v1::CreateCollectionRequest request;
		request.set_id("id:GrpcTest.CreateCollection");
		request.set_name("name:GrpcTest.CreateCollection");

		auto reactor = service.CreateCollection(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);
		EXPECT_EQ(request.id(), response.id());
		EXPECT_EQ(request.name(), response.name());
	}

	// Error: duplicate `id`
	{
		const datastore::Collection collection({.name = "name:GrpcTest.CreateCollection"});
		EXPECT_NO_THROW(collection.store());

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Collection                    response;

		gk::v1::CreateCollectionRequest request;
		request.set_id(collection.id());
		request.set_name("name:GrpcTest.CreateCollection-duplicate");

		auto reactor = service.CreateCollection(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_EQ(grpc::StatusCode::ALREADY_EXISTS, peer.test_status().error_code());
		EXPECT_EQ("Duplicate collection id", peer.test_status().error_message());
	}
}

TEST_F(GrpcTest, RetrieveCollection) {
	service::Grpc service;

	// Success: retrieve collection by id
	{
		const datastore::Collection collection(
			{.id = "id:GrpcTest.RetrieveCollection", .name = "name:GrpcTest.RetrieveCollection"});
		EXPECT_NO_THROW(collection.store());

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Collection                    response;

		gk::v1::RetrieveCollectionRequest request;
		request.set_id(collection.id());

		auto reactor = service.RetrieveCollection(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);
		EXPECT_EQ(collection.id(), response.id());
		EXPECT_EQ(collection.name(), response.name());
	}

	// Error: collection not found
	{
		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Collection                    response;

		gk::v1::RetrieveCollectionRequest request;
		request.set_id("id:GrpcTest.RetrieveCollection-not-found");

		auto reactor = service.RetrieveCollection(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_EQ(grpc::StatusCode::NOT_FOUND, peer.test_status().error_code());
		EXPECT_EQ("Document not found", peer.test_status().error_message());
	}
}

TEST_F(GrpcTest, UpdateCollection) {
	service::Grpc service;

	// Success: update collection name
	{
		const datastore::Collection collection(
			{.id = "id:GrpcTest.UpdateCollection-name", .name = "name:GrpcTest.UpdateCollection"});
		EXPECT_NO_THROW(collection.store());

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Collection                    response;

		gk::v1::UpdateCollectionRequest request;
		request.set_id(collection.id());
		request.set_name("name:GrpcTest.UpdateCollection-new");

		auto reactor = service.UpdateCollection(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);

		EXPECT_EQ(request.id(), response.id());
		EXPECT_EQ(request.name(), response.name());

		auto updatedCollection = datastore::RetrieveCollection(collection.id());
		EXPECT_EQ(request.name(), updatedCollection.name());
		EXPECT_EQ(1, updatedCollection.rev());
	}

	// Error: no fields to update
	{
		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Collection                    response;

		gk::v1::UpdateCollectionRequest request;
		request.set_id("id:GrpcTest.UpdateCollection-no-updates");

		auto reactor = service.UpdateCollection(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_EQ(grpc::StatusCode::INTERNAL, peer.test_status().error_code());
		EXPECT_EQ("No fields to update", peer.test_status().error_message());
	}

	// Error: collection not found
	{
		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Collection                    response;

		gk::v1::UpdateCollectionRequest request;
		request.set_id("id:GrpcTest.UpdateCollection-not-found");
		request.set_name("name:GrpcTest.UpdateCollection-not-found");

		auto reactor = service.UpdateCollection(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_EQ(grpc::StatusCode::NOT_FOUND, peer.test_status().error_code());
		EXPECT_EQ("Document not found", peer.test_status().error_message());
	}
}

// Collections - members
TEST_F(GrpcTest, AddCollectionMember) {
	datastore::Collection collection({.name = "name:GrpcTest.AddCollectionMember"});
	ASSERT_NO_THROW(collection.store());

	service::Grpc service;

	// Success: add collection member
	{
		datastore::Identity identity({.sub="sub:GrpcTest.AddCollectionMember"});
		ASSERT_NO_THROW(identity.store());

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::AddCollectionMemberResponse   response;

		gk::v1::AddCollectionMemberRequest request;
		request.set_collection_id(collection.id());
		request.set_identity_id(identity.id());

		auto reactor = service.AddCollectionMember(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);
	}
}

// Identities
TEST_F(GrpcTest, CreateIdentity) {
	service::Grpc service;

	// Success: create identity
	{
		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Identity                      response;

		gk::v1::CreateIdentityRequest request;
		request.set_sub("sub:GrpcTest.CreateIdentity");

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
		request.set_id("id:GrpcTest.CreateIdentity-with_id");
		request.set_sub("sub:GrpcTest.CreateIdentity-with_id");

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
		request.set_sub("sub:GrpcTest.CreateIdentity-with_attrs");

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
			.sub = "sub:GrpcTest.CreateIdentity-duplicate_id",
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
		request.set_sub("sub:GrpcTest.CreateIdentity-duplicate_id");

		auto reactor = service.CreateIdentity(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_EQ(grpc::StatusCode::ALREADY_EXISTS, peer.test_status().error_code());
		EXPECT_EQ("Duplicate identity id", peer.test_status().error_message());
	}

	// Error: duplicate `sub`
	{
		const datastore::Identity identity({
			.sub = "sub:GrpcTest.CreateIdentity-duplicate",
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

TEST_F(GrpcTest, RetrieveIdentity) {
	service::Grpc service;

	// Success: retrieve identity by id
	{
		const datastore::Identity identity(
			{.id = "id:GrpcTest.RetrieveIdentity", .sub = "sub:GrpcTest.RetrieveIdentity"});
		EXPECT_NO_THROW(identity.store());

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

	// Success: retrieve identity by id with attrs
	{
		const datastore::Identity identity({
			.attrs = R"({"flag":true})",
			.sub   = "sub:GrpcTest.RetrieveIdentity-with_attrs",
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
		request.set_id("id:GrpcTest.RetrieveIdentity-not-found");

		auto reactor = service.RetrieveIdentity(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_EQ(grpc::StatusCode::NOT_FOUND, peer.test_status().error_code());
		EXPECT_EQ("Document not found", peer.test_status().error_message());
	}
}

TEST_F(GrpcTest, UpdateIndentity) {
	service::Grpc service;

	// Success: update identity sub
	{
		const datastore::Identity identity(
			{.id = "id:GrpcTest.UpdateIdentity-sub", .sub = "sub:GrpcTest.UpdateIdentity-sub"});
		ASSERT_NO_THROW(identity.store());

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Identity                      response;

		gk::v1::UpdateIdentityRequest request;
		request.set_id(identity.id());
		request.set_sub("sub:GrpcTest.UpdateIdentity-new");

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
		const datastore::Identity identity({.sub = "sub:GrpcTest.UpdateIdentity-attrs"});
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
		request.set_id("id:GrpcTest.UpdateIdentity-no-updates");

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
		request.set_id("id:GrpcTest.UpdateIdentity-not-found");
		request.set_sub("name:GrpcTest.UpdateIdentity-not-found");

		auto reactor = service.UpdateIdentity(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_EQ(grpc::StatusCode::NOT_FOUND, peer.test_status().error_code());
		EXPECT_EQ("Document not found", peer.test_status().error_message());
	}
}
