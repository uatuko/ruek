#include <grpcpp/test/default_reactor_test_peer.h>
#include <gtest/gtest.h>

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

TEST_F(GrpcTest, CreateCollection) {
	service::Grpc service;

	// Success: create collection
	{
		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Collection                    response;

		gk::v1::CreateCollectionRequest request;
		request.set_name("sub:GrpcTest.CreateCollection");

		auto reactor = service.CreateCollection(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);
		EXPECT_EQ(request.name(), response.name());
		EXPECT_FALSE(response.id().empty());
	}
}

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
	}

	// Error: duplicate identity
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
