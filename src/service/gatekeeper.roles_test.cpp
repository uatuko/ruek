#include "gatekeeper.h"

#include <grpcpp/test/default_reactor_test_peer.h>
#include <gtest/gtest.h>

#include "datastore/roles.h"
#include "datastore/testing.h"

class GatekeeperRolesTest : public testing::Test {
protected:
	static void SetUpTestSuite() {
		datastore::testing::setup();

		// Clear data
		datastore::pg::exec("truncate table roles cascade;");
	}

	static void TearDownTestSuite() { datastore::testing::teardown(); }
};

TEST_F(GatekeeperRolesTest, CreateRole) {
	service::Gatekeeper service;

	// Success: create role
	{
		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Role                          response;

		gk::v1::CreateRoleRequest request;
		request.set_name("name:GatekeeperTest.CreateRole");
		request.add_permissions("permissions[0]:GatekeeperTest.CreateRole");

		auto reactor = service.CreateRole(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);

		EXPECT_FALSE(response.id().empty());
		EXPECT_EQ(request.name(), response.name());
		for (int i = 0; i < request.permissions_size(); i++) {
			EXPECT_EQ(request.permissions(i), response.permissions(i));
		}
	}
}

TEST_F(GatekeeperRolesTest, RetrieveRole) {
	service::Gatekeeper service;

	// Success: retrieve role
	{
		const datastore::Role role({
			.id   = "id:GatekeeperTest.RetrieveRole",
			.name = "name:GatekeeperTest.RetrieveRole",
			.permissions =
				{
					{"permissions[0]:GatekeeperTest.RetrieveRole"},
					{"permissions[1]:GatekeeperTest.RetrieveRole"},
				},
		});
		ASSERT_NO_THROW(role.store());

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Role                          response;

		gk::v1::RetrieveRoleRequest request;
		request.set_id(role.id());

		auto reactor = service.RetrieveRole(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);

		EXPECT_EQ(role.id(), response.id());
		EXPECT_EQ(role.name(), response.name());

		const auto &perms = role.permissions();
		ASSERT_EQ(perms.size(), response.permissions_size());
		for (int i = 0; i < response.permissions_size(); i++) {
			EXPECT_EQ(1, perms.count(response.permissions(i)));
		}
	}
}
