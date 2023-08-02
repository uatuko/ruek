#include <grpcpp/test/default_reactor_test_peer.h>
#include <gtest/gtest.h>

#include "datastore/collections.h"
#include "datastore/identities.h"
#include "datastore/permissions.h"
#include "datastore/rbac-policies.h"
#include "datastore/roles.h"
#include "datastore/testing.h"

#include "rbac.h"

class svc_RbacTest : public testing::Test {
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

TEST_F(svc_RbacTest, Check) {
	svc::Rbac svc;

	// Success: returns policy when found
	{
		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::RbacCheckResponse             response;

		const datastore::Identity identity({
			.sub = "sub:svc_RbacTest.Check",
		});
		ASSERT_NO_THROW(identity.store());

		datastore::RbacPolicy policy({
			.name = "name::svc_RbacTest.Check",
		});
		ASSERT_NO_THROW(policy.store());

		const auto permission = "permission:svc_RbacTest.Check";

		const datastore::RbacPolicy::Cache cache({
			.identity   = identity.id(),
			.permission = permission,
			.policy     = policy.id(),
			.rule       = {},
		});
		ASSERT_NO_THROW(cache.store());

		gk::v1::RbacCheckRequest request;
		request.set_permission(permission);
		request.set_identity_id(identity.id());

		auto reactor = svc.Check(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);
		EXPECT_EQ(1, response.policies().size());
	}
}

TEST_F(svc_RbacTest, CreatePolicy) {
	svc::Rbac svc;
	// Success: create rbac policy with `id`
	{
		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::RbacPolicy                    response;

		gk::v1::RbacCreatePolicyRequest request;
		request.set_id("id:svc_RbacTest.CreatePolicy-with_id");
		request.set_name("name:svc_RbacTest.CreatePolicy-with_id");

		auto reactor = svc.CreatePolicy(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);
		EXPECT_EQ(request.id(), response.id());
		EXPECT_EQ(request.name(), response.name());
	}

	// Error: duplicate `id`
	{
		const datastore::RbacPolicy policy({
			.name = "name:svc_RbacTest.CreatePolicy-duplicate_id",
		});
		ASSERT_NO_THROW(policy.store());

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::RbacPolicy                    response;

		gk::v1::RbacCreatePolicyRequest request;
		request.set_id(policy.id());
		request.set_name("name:svc_RbacTest.CreatePolicy-duplicate_id");

		auto reactor = svc.CreatePolicy(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_EQ(grpc::StatusCode::ALREADY_EXISTS, peer.test_status().error_code());
		EXPECT_EQ("Duplicate policy id", peer.test_status().error_message());
	}

	// Success: create rbac policy with identity and role
	{
		const datastore::Identity identity({
			.sub = "sub:svc_RbacTest.CreatePolicy-with_identity_and_role",
		});
		ASSERT_NO_THROW(identity.store());

		const datastore::Permission permission({
			.id = "permissions[0].id:vc_RbacTest.CreateRbacPolicy-with_identity_and_rol",
		});
		ASSERT_NO_THROW(permission.store());

		const datastore::Role role({
			.name = "name:svc_RbacTest.CreatePolicy-with_identity_and_role",
		});
		ASSERT_NO_THROW(role.store());
		ASSERT_NO_THROW(role.addPermission(permission.id()));

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::RbacPolicy                    response;

		gk::v1::RbacCreatePolicyRequest request;
		request.set_name("name:svc_RbacTest.CreatePolicy-with_identity_and_role");
		request.add_identity_ids(identity.id());

		auto rule = request.add_rules();
		rule->set_role_id(role.id());

		// expect no access before request
		{
			const auto policies =
				datastore::RbacPolicy::Cache::check(identity.id(), permission.id());
			EXPECT_EQ(0, policies.size());
		}

		auto reactor = svc.CreatePolicy(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);

		EXPECT_FALSE(response.id().empty());
		EXPECT_EQ(request.name(), response.name());
		EXPECT_EQ(identity.id(), response.identity_ids(0));
		EXPECT_EQ(role.id(), response.rules(0).role_id());

		// expect to find single policy when checking access
		{
			const auto policies =
				datastore::RbacPolicy::Cache::check(identity.id(), permission.id());
			EXPECT_EQ(1, policies.size());
		}
	}

	// Success: create an rbac policy with collection and role
	{
		const datastore::Identity identity({
			.sub = "sub:svc_RbacTest.CreatePolicy-with_collection_and_role",
		});
		ASSERT_NO_THROW(identity.store());
		const datastore::Collection collection({
			.name = "name:svc_RbacTest.CreatePolicy-with_collection_and_role",
		});
		ASSERT_NO_THROW(collection.store());
		ASSERT_NO_THROW(collection.add(identity.id()));

		const datastore::Permission permission({
			.id = "permissions[0].id:svc_RbacTest.CreatePolicy-with_collection_and_role",
		});
		ASSERT_NO_THROW(permission.store());

		const datastore::Role role({
			.name = "name:svc_RbacTest.CreatePolicy-with_collection_and_role",
		});
		ASSERT_NO_THROW(role.store());
		ASSERT_NO_THROW(role.addPermission(permission.id()));

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::RbacPolicy                    response;

		gk::v1::RbacCreatePolicyRequest request;
		request.set_name("name:svc_RbacTest.CreatePolicy-with_collection_and_role");
		request.add_collection_ids(collection.id());

		auto rule = request.add_rules();
		rule->set_role_id(role.id());

		// expect no permission before request
		{
			const auto policies =
				datastore::RbacPolicy::Cache::check(identity.id(), permission.id());
			EXPECT_EQ(0, policies.size());
		}

		// create rbac policy
		auto reactor = svc.CreatePolicy(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);

		EXPECT_FALSE(response.id().empty());
		EXPECT_EQ(request.name(), response.name());
		EXPECT_EQ(collection.id(), response.collection_ids(0));
		EXPECT_EQ(role.id(), response.rules(0).role_id());

		// expect to find single policy when checking
		{
			const auto policies =
				datastore::RbacPolicy::Cache::check(identity.id(), permission.id());
			EXPECT_EQ(1, policies.size());
		}
	}
}

TEST_F(svc_RbacTest, RetrievePolicy) {
	svc::Rbac svc;

	// Success: retrieve data
	{
		const datastore::Identities identities({
			{{.sub = "sub:svc_RbacTest.RetrievePolicy[0]"}},
			{{.sub = "sub:svc_RbacTest.RetrievePolicy[1]"}},
		});

		for (const auto &idn : identities) {
			ASSERT_NO_THROW(idn.store());
		}

		const datastore::Collection collection({
			.name = "name:svc_RbacTest.RetrievePolicy",
		});
		ASSERT_NO_THROW(collection.store());
		ASSERT_NO_THROW(collection.add(identities[0].id()));

		const datastore::RbacPolicy policy({
			.id   = "id:svc_RbacTest.RetrievePolicy",
			.name = "name:svc_RbacTest.RetrievePolicy",
		});
		ASSERT_NO_THROW(policy.store());
		ASSERT_NO_THROW(policy.addCollection(collection.id()));
		ASSERT_NO_THROW(policy.addIdentity(identities[1].id()));

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::RbacPolicy                    response;

		gk::v1::RbacRetrievePolicyRequest request;
		request.set_id(policy.id());

		auto reactor = svc.RetrievePolicy(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);

		EXPECT_EQ(policy.id(), response.id());
		EXPECT_EQ(policy.name(), response.name());

		ASSERT_EQ(1, response.collection_ids_size());
		EXPECT_EQ(collection.id(), response.collection_ids(0));

		ASSERT_EQ(1, response.identity_ids_size());
		EXPECT_EQ(identities[1].id(), response.identity_ids(0));
	}
}
