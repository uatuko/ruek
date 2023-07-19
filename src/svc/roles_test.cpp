#include <grpcpp/test/default_reactor_test_peer.h>
#include <gtest/gtest.h>

#include "datastore/roles.h"
#include "datastore/testing.h"

#include "roles.h"

class svc_RolesTest : public testing::Test {
protected:
	static void SetUpTestSuite() {
		datastore::testing::setup();

		// Clear data
		datastore::pg::exec("truncate table roles cascade;");
	}

	static void TearDownTestSuite() { datastore::testing::teardown(); }
};

TEST_F(svc_RolesTest, Create) {
	svc::Roles svc;

	// Success: create role
	{
		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Role                          response;

		gk::v1::CreateRoleRequest request;
		request.set_name("name:svc_RolesTest.CreateRole");
		request.add_permissions("permissions[0]:svc_RolesTest.CreateRole");

		auto reactor = svc.Create(&ctx, &request, &response);
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

TEST_F(svc_RolesTest, Retrieve) {
	svc::Roles svc;

	// Success: retrieve role
	{
		const datastore::Role role({
			.id   = "id:svc_RolesTest.RetrieveRole",
			.name = "name:svc_RolesTest.RetrieveRole",
			.permissions =
				{
					{"permissions[0]:svc_RolesTest.RetrieveRole"},
					{"permissions[1]:svc_RolesTest.RetrieveRole"},
				},
		});
		ASSERT_NO_THROW(role.store());

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Role                          response;

		gk::v1::RetrieveRoleRequest request;
		request.set_id(role.id());

		auto reactor = svc.Retrieve(&ctx, &request, &response);
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
