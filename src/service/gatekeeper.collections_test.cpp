#include "gatekeeper.h"

#include <grpcpp/test/default_reactor_test_peer.h>
#include <gtest/gtest.h>

#include "datastore/collections.h"
#include "datastore/identities.h"
#include "datastore/testing.h"

class GatekeeperCollectionsTest : public testing::Test {
protected:
	static void SetUpTestSuite() {
		datastore::testing::setup();

		// Clear data
		datastore::pg::exec("truncate table collections cascade;");
		datastore::pg::exec("truncate table identities cascade;");
	}

	static void TearDownTestSuite() { datastore::testing::teardown(); }
};

TEST_F(GatekeeperCollectionsTest, CreateCollection) {
	service::Gatekeeper service;

	// Success: create collection
	{
		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Collection                    response;

		gk::v1::CreateCollectionRequest request;
		request.set_name("name:GatekeeperTest.CreateCollection");

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
		request.set_id("id:GatekeeperTest.CreateCollection");
		request.set_name("name:GatekeeperTest.CreateCollection");

		auto reactor = service.CreateCollection(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);
		EXPECT_EQ(request.id(), response.id());
		EXPECT_EQ(request.name(), response.name());
	}

	// Error: duplicate `id`
	{
		const datastore::Collection collection({.name = "name:GatekeeperTest.CreateCollection"});
		EXPECT_NO_THROW(collection.store());

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Collection                    response;

		gk::v1::CreateCollectionRequest request;
		request.set_id(collection.id());
		request.set_name("name:GatekeeperTest.CreateCollection-duplicate");

		auto reactor = service.CreateCollection(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_EQ(grpc::StatusCode::ALREADY_EXISTS, peer.test_status().error_code());
		EXPECT_EQ("Duplicate collection id", peer.test_status().error_message());
	}
}

TEST_F(GatekeeperCollectionsTest, RetrieveCollection) {
	service::Gatekeeper service;

	// Success: retrieve collection by id
	{
		const datastore::Collection collection(
			{.id   = "id:GatekeeperTest.RetrieveCollection",
			 .name = "name:GatekeeperTest.RetrieveCollection"});
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
		request.set_id("id:GatekeeperTest.RetrieveCollection-not-found");

		auto reactor = service.RetrieveCollection(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_EQ(grpc::StatusCode::NOT_FOUND, peer.test_status().error_code());
		EXPECT_EQ("Document not found", peer.test_status().error_message());
	}
}

TEST_F(GatekeeperCollectionsTest, UpdateCollection) {
	service::Gatekeeper service;

	// Success: update collection name
	{
		const datastore::Collection collection(
			{.id   = "id:GatekeeperTest.UpdateCollection-name",
			 .name = "name:GatekeeperTest.UpdateCollection"});
		EXPECT_NO_THROW(collection.store());

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Collection                    response;

		gk::v1::UpdateCollectionRequest request;
		request.set_id(collection.id());
		request.set_name("name:GatekeeperTest.UpdateCollection-new");

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
		request.set_id("id:GatekeeperTest.UpdateCollection-no-updates");

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
		request.set_id("id:GatekeeperTest.UpdateCollection-not-found");
		request.set_name("name:GatekeeperTest.UpdateCollection-not-found");

		auto reactor = service.UpdateCollection(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_EQ(grpc::StatusCode::NOT_FOUND, peer.test_status().error_code());
		EXPECT_EQ("Document not found", peer.test_status().error_message());
	}
}

// Members
TEST_F(GatekeeperCollectionsTest, AddCollectionMember) {
	datastore::Collection collection({.name = "name:GatekeeperTest.AddCollectionMember"});
	ASSERT_NO_THROW(collection.store());

	service::Gatekeeper service;

	// Success: add collection member
	{
		datastore::Identity identity({.sub = "sub:GatekeeperTest.AddCollectionMember"});
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

		{
			std::string_view qry = R"(
					select count(*)
					from collections_identities
					where
						collection_id = $1::text
						and identity_id = $2::text;
				)";

			auto res = datastore::pg::exec(qry, collection.id(), identity.id());
			ASSERT_EQ(1, res.size());

			auto [count] = res[0].as<int>();
			EXPECT_EQ(1, count);
		}
	}
}

TEST_F(GatekeeperCollectionsTest, ListCollectionMembers) {
	datastore::Collection collection({.name = "name:GatekeeperTest.ListCollectionMembers"});
	ASSERT_NO_THROW(collection.store());

	service::Gatekeeper service;

	// Success: list collection members
	{
		std::array<datastore::Identity, 2> identities = {
			datastore::Identity({.sub = "sub:GatekeeperTest.ListCollectionMembers-1"}),
			datastore::Identity({.sub = "sub:GatekeeperTest.ListCollectionMembers-2"}),
		};

		for (const auto &idn : identities) {
			ASSERT_NO_THROW(idn.store());
		}

		ASSERT_NO_THROW(collection.add(identities[0].id()));

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::ListCollectionMembersResponse response;

		gk::v1::ListCollectionMembersRequest request;
		request.set_id(collection.id());

		auto reactor = service.ListCollectionMembers(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);

		EXPECT_FALSE(response.has_meta());

		ASSERT_EQ(1, response.data_size());
		EXPECT_EQ(identities[0].id(), response.data(0).id());
	}
}

TEST_F(GatekeeperCollectionsTest, RemoveCollectionMember) {
	datastore::Collection collection({.name = "name:GatekeeperTest.RemoveCollectionMember"});
	ASSERT_NO_THROW(collection.store());

	service::Gatekeeper service;

	// Success: remove collection member
	{
		datastore::Identity identity({.sub = "sub:GatekeeperTest.RemoveCollectionMember"});
		ASSERT_NO_THROW(identity.store());
		ASSERT_NO_THROW(collection.add(identity.id()));

		grpc::CallbackServerContext            ctx;
		grpc::testing::DefaultReactorTestPeer  peer(&ctx);
		gk::v1::RemoveCollectionMemberResponse response;

		gk::v1::RemoveCollectionMemberRequest request;
		request.set_collection_id(collection.id());
		request.set_identity_id(identity.id());

		auto reactor = service.RemoveCollectionMember(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);

		{
			std::string_view qry = R"(
					select count(*)
					from collections_identities
					where
						collection_id = $1::text
						and identity_id = $2::text;
				)";

			auto res = datastore::pg::exec(qry, collection.id(), identity.id());
			ASSERT_EQ(1, res.size());

			auto [count] = res[0].as<int>();
			EXPECT_EQ(0, count);
		}
	}
}
