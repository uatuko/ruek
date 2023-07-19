#include <grpcpp/test/default_reactor_test_peer.h>
#include <gtest/gtest.h>

#include "datastore/access-policies.h"
#include "datastore/collections.h"
#include "datastore/identities.h"
#include "datastore/rbac-policies.h"
#include "datastore/roles.h"
#include "datastore/testing.h"

#include "gatekeeper.h"

class GatekeeperTest : public testing::Test {
protected:
	static void SetUpTestSuite() {
		datastore::testing::setup();

		// Clear data
		datastore::pg::exec("truncate table \"access-policies\" cascade;");
		datastore::pg::exec("truncate table collections cascade;");
		datastore::pg::exec("truncate table identities cascade;");
		datastore::pg::exec("truncate table \"rbac-policies\" cascade;");
		datastore::pg::exec("truncate table roles cascade;");
		datastore::redis::conn().cmd("flushall");
	}

	static void TearDownTestSuite() { datastore::testing::teardown(); }
};

TEST_F(GatekeeperTest, CheckRbac) {
	service::Gatekeeper service;

	// Success: returns policy when found
	{
		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::CheckRbacResponse             response;

		// create identity
		const datastore::Identity identity({.sub = "sub:GatekeeperTest.CheckRbac"});
		ASSERT_NO_THROW(identity.store());

		datastore::RbacPolicy policy({
			.name = "name::GatekeeperTest.CheckRbac",
		});
		ASSERT_NO_THROW(policy.store());

		const auto permission = "permission:GatekeeperTest.CheckRbac";

		const datastore::RbacPolicy::Cache cache({
			.identity   = identity.id(),
			.permission = permission,
			.policy     = policy.id(),
			.rule       = {},
		});
		ASSERT_NO_THROW(cache.store());

		gk::v1::CheckRbacRequest request;
		request.set_permission(permission);
		request.set_identity_id(identity.id());

		auto reactor = service.CheckRbac(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);
		EXPECT_EQ(1, response.policies().size());
	}
}
