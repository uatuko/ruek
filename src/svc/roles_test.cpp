#include <grpcpp/test/default_reactor_test_peer.h>
#include <gtest/gtest.h>

#include "datastore/identities.h"
#include "datastore/permissions.h"
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

TEST_F(svc_RolesTest, AddPermission) {
	svc::Roles svc;

	// Success: add permission
	{
		// Create Identity
		const datastore::Identity identity({
			.id  = "identity_id:svc_RolesTest.AddPermission",
			.sub = "identity_sub:svc_RolesTest.AddPermission",
		});
		ASSERT_NO_THROW(identity.store());

		// Create permission
		datastore::Permission permission1({
			.id = "permissions[0].id:svc_RolesTest.AddPermission",
		});
		ASSERT_NO_THROW(permission1.store());

		// Create role
		const datastore::Role role({
			.id   = "id:GatekeeperTest.AddPermission",
			.name = "name:GatekeeperTest.AddPermission",

		});
		ASSERT_NO_THROW(role.store());

		// Create policy
		const datastore::RbacPolicy policy({
			.name = "policy_name:svc_RolesTest.AddPermission",
		});
		ASSERT_NO_THROW(policy.store());
		ASSERT_NO_THROW(policy.addIdentity(identity.id()));
		ASSERT_NO_THROW(policy.addRule({
			.roleId = role.id(),
		}));
		ASSERT_EQ(policy.rules().size(), 1);

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Role                          response;

		gk::v1::RolesAddPermissionRequest request;
		request.set_role_id(role.id());
		request.set_id(permission1.id());

		// Add new permission
		auto reactor = svc.AddPermission(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);

		// Check response
		EXPECT_EQ(role.id(), response.id());
		EXPECT_EQ(role.name(), response.name());
		EXPECT_EQ(1, response.permissions().size());

		// Ensure permissions included
		const auto &perms = datastore::RetrievePermissionsByRole(role.id());
		ASSERT_EQ(perms.size(), response.permissions_size());
		ASSERT_EQ(perms[0].id(), response.permissions(0).id());

		// Expect to find single policy when checking RBAC
		{
			const auto policies =
				datastore::RbacPolicy::Cache::check(identity.id(), permission1.id());
			EXPECT_EQ(1, policies.size());
		}
	}
}

TEST_F(svc_RolesTest, Create) {
	svc::Roles svc;

	// Success: create role
	{
		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Role                          response;

		datastore::Permission permission({
			.id = "permission_id:svc_RolesTest.Create",
		});
		ASSERT_NO_THROW(permission.store());

		gk::v1::RolesCreateRequest request;
		request.set_name("name:svc_RolesTest.Create");
		request.add_permission_ids(permission.id());

		auto reactor = svc.Create(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);

		EXPECT_FALSE(response.id().empty());
		EXPECT_EQ(request.name(), response.name());
		for (int i = 0; i < request.permission_ids_size(); i++) {
			EXPECT_EQ(request.permission_ids(i), response.permissions(i).id());
		}
	}
}

TEST_F(svc_RolesTest, Retrieve) {
	svc::Roles svc;

	// Success: retrieve role
	{
		datastore::Permission permission1({
			.id = "permissions[0].id:svc_RolesTest.RetrieveRole",
		});
		datastore::Permission permission2({
			.id = "permissions[1].id:svc_RolesTest.RetrieveRole",
		});
		ASSERT_NO_THROW(permission1.store());
		ASSERT_NO_THROW(permission2.store());
		const datastore::Role role({
			.id   = "id:GatekeeperTest.RetrieveRole",
			.name = "name:GatekeeperTest.RetrieveRole",

		});
		ASSERT_NO_THROW(role.store());
		ASSERT_NO_THROW(role.addPermission(permission1.id()));
		ASSERT_NO_THROW(role.addPermission(permission2.id()));

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Role                          response;

		gk::v1::RolesRetrieveRequest request;
		request.set_id(role.id());

		auto reactor = svc.Retrieve(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);

		EXPECT_EQ(role.id(), response.id());
		EXPECT_EQ(role.name(), response.name());

		const auto &perms = datastore::RetrievePermissionsByRole(role.id());
		ASSERT_EQ(perms.size(), response.permissions_size());
		ASSERT_EQ(perms[0].id(), response.permissions(0).id());
		ASSERT_EQ(perms[1].id(), response.permissions(1).id());
	}
}
