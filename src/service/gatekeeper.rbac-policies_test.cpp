#include "gatekeeper.h"

#include <grpcpp/test/default_reactor_test_peer.h>
#include <gtest/gtest.h>

#include "datastore/collections.h"
#include "datastore/identities.h"
#include "datastore/rbac-policies.h"
#include "datastore/roles.h"
#include "datastore/testing.h"

class GatekeeperRbacPoliciesTest : public testing::Test {
protected:
	static void SetUpTestSuite() {
		datastore::testing::setup();

		// Clear data
		datastore::pg::exec("truncate table collections cascade;");
		datastore::pg::exec("truncate table identities cascade;");
		datastore::pg::exec("truncate table \"rbac-policies\" cascade;");
		datastore::pg::exec("truncate table roles cascade;");
		datastore::redis::conn().cmd("flushall");
	}

	static void TearDownTestSuite() { datastore::testing::teardown(); }
};

TEST_F(GatekeeperRbacPoliciesTest, CreateRbacPolicy) {
	service::Gatekeeper service;
	// Success: create rbac policy with `id`
	{
		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::RbacPolicy                    response;

		gk::v1::CreateRbacPolicyRequest request;
		request.set_id("id:GatekeeperTest.CreateRbacPolicy-id");
		request.set_name("name:GatekeeperTest.CreateRbacPolicy-id");

		auto reactor = service.CreateRbacPolicy(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);
		EXPECT_EQ(request.id(), response.id());
		EXPECT_EQ(request.name(), response.name());
	}

	// Error: duplicate `id`
	{
		const datastore::RbacPolicy policy(
			{.name = "name:GatekeeperTest.CreateRbacPolicy-duplicate"});
		EXPECT_NO_THROW(policy.store());

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::RbacPolicy                    response;

		gk::v1::CreateRbacPolicyRequest request;
		request.set_id(policy.id());
		request.set_name("name:GatekeeperTest.CreateRbacPolicy-duplicate");

		auto reactor = service.CreateRbacPolicy(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_EQ(grpc::StatusCode::ALREADY_EXISTS, peer.test_status().error_code());
		EXPECT_EQ("Duplicate policy id", peer.test_status().error_message());
	}

	// Success: create rbac policy
	{
		const datastore::Identity identity({.sub = "sub:GatekeeperTest.CreateRbacPolicy"});
		ASSERT_NO_THROW(identity.store());
		auto                  permission = "permissions[0]:GatekeeperTest.CreateRbacRbacPolicy";
		const datastore::Role role({
			.name = "name:GatekeeperTest.CreateRbacPolicy",
			.permissions =
				{
					permission,
				},
		});
		ASSERT_NO_THROW(role.store());

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::RbacPolicy                    response;

		gk::v1::CreateRbacPolicyRequest request;
		request.set_name("name:GatekeeperTest.CreateRbacPolicy");
		request.add_identity_ids(identity.id());

		auto rule = request.add_rules();
		rule->set_role_id(role.id());

		// expect no access before request
		{
			const auto policies = datastore::RbacPolicy::Cache::check(identity.id(), permission);
			EXPECT_EQ(0, policies.size());
		}

		auto reactor = service.CreateRbacPolicy(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);

		EXPECT_FALSE(response.id().empty());
		EXPECT_EQ(request.name(), response.name());
		EXPECT_EQ(identity.id(), response.identity_ids(0));
		EXPECT_EQ(role.id(), response.rules(0).role_id());

		// expect to find single policy when checking access
		{
			const auto policies = datastore::RbacPolicy::Cache::check(identity.id(), permission);
			EXPECT_EQ(1, policies.size());
		}
	}

	// Success: create an rbac policy for collection
	// all members of collection should be granted access
	{
		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::RbacPolicy                    response;

		const datastore::Identity identity({
			.sub = "sub:GatekeeperTest.CreateRbacPolicy-collection",
		});
		ASSERT_NO_THROW(identity.store());
		const datastore::Collection collection({
			.name = "name:GatekeeperTest.CreateRbacPolicy-collection",
		});
		ASSERT_NO_THROW(collection.store());
		ASSERT_NO_THROW(collection.add(identity.id()));

		auto permission = "permissions[0]:GatekeeperTest.CreateRbacPolicy-collection";
		const datastore::Role role({
			.name = "name:GatekeeperTest.CreateRbacPolicy",
			.permissions =
				{
					permission,
				},
		});
		ASSERT_NO_THROW(role.store());

		gk::v1::CreateRbacPolicyRequest request;
		request.set_name("name:GatekeeperTest.CreateRbacPolicy");
		request.add_collection_ids(collection.id());

		auto rule = request.add_rules();
		rule->set_role_id(role.id());

		// expect no access before request
		{
			const auto policies = datastore::RbacPolicy::Cache::check(identity.id(), permission);
			EXPECT_EQ(0, policies.size());
		}

		// create access policy
		auto reactor = service.CreateRbacPolicy(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);

		EXPECT_FALSE(response.id().empty());
		EXPECT_EQ(request.name(), response.name());
		EXPECT_EQ(collection.id(), response.collection_ids(0));
		EXPECT_EQ(role.id(), response.rules(0).role_id());

		// expect to find single policy when checking access
		{
			const auto policies = datastore::RbacPolicy::Cache::check(identity.id(), permission);
			EXPECT_EQ(1, policies.size());
		}
	}

	// FIXME: nested collections
}
