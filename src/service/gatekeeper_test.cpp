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

// Access control checks
TEST_F(GatekeeperTest, CheckAccess) {
	service::Gatekeeper service;

	// Success: returns policy when found
	{
		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::CheckAccessResponse           response;

		const datastore::Identity identity({
			.sub = "sub:GatekeeperTest.CheckAccess",
		});
		ASSERT_NO_THROW(identity.store());

		const datastore::AccessPolicy policy({
			.name = "name:GatekeeperTest.CheckAccess",
		});
		ASSERT_NO_THROW(policy.store());

		const std::string resource = "resource/GatekeeperTest.CheckAccess";

		const datastore::AccessPolicy::Cache cache({
			.identity = identity.id(),
			.policy   = policy.id(),
			.rule     = {.resource = resource},
		});
		ASSERT_NO_THROW(cache.store());

		gk::v1::CheckAccessRequest request;
		request.set_resource(resource);
		request.set_identity_id(identity.id());

		auto reactor = service.CheckAccess(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);
		EXPECT_EQ(1, response.policies().size());
	}
}

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

// Events
TEST_F(GatekeeperTest, ConsumeEvent_cache_rebuild) {
	service::Gatekeeper service;

	// Success: request/cache.rebuild:access
	{
		const datastore::Identity identity({
			.sub = "sub:GatekeeperTest.ConsumeEvent(request/cache.rebuild:access)",
		});
		ASSERT_NO_THROW(identity.store());

		const datastore::Collection collection({
			.name = "name:GatekeeperTest.ConsumeEvent(request/cache.rebuild:access)",
		});
		ASSERT_NO_THROW(collection.store());
		ASSERT_NO_THROW(collection.add(identity.id()));

		const datastore::AccessPolicy policy({
			.name = "name:GatekeeperTest.ConsumeEvent(request/cache.rebuild:access)",
			.rules =
				{
					{
						.attrs    = "attrs(request/cache.rebuild:access)",
						.resource = "resource(request/cache.rebuild:access)",
					},
				},
		});
		ASSERT_NO_THROW(policy.store());
		ASSERT_NO_THROW(policy.addCollection(collection.id()));

		const auto identityIds = policy.identities(true);
		EXPECT_EQ(1, identityIds.size());

		// Ensure cache is clear
		{
			for (const auto &id : identityIds) {
				for (const auto &rule : policy.rules()) {
					const datastore::AccessPolicy::Cache cache({
						.identity = id,
						.policy   = policy.id(),
						.rule     = rule,
					});

					ASSERT_NO_THROW(cache.discard());
				}
			}
		}

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::ConsumeEventResponse          response;

		gk::v1::RebuildAccessCacheEventPayload payload;
		payload.add_ids(policy.id());

		gk::v1::Event request;
		request.set_name("request/cache.rebuild:access");

		auto any = request.mutable_payload();
		any->PackFrom(payload);

		auto reactor = service.ConsumeEvent(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);

		// Expect cache to be rebuilt
		{
			for (const auto &id : identityIds) {
				for (const auto &rule : policy.rules()) {
					const auto policies = datastore::AccessPolicy::Cache::check(id, rule.resource);

					ASSERT_EQ(1, policies.size());
					EXPECT_EQ(policy.id(), policies[0].id);
					EXPECT_EQ(rule.attrs, policies[0].attrs);
				}
			}
		}
	}

	// Success: request/cache.rebuild:rbac
	{
		const datastore::Identity identity({
			.sub = "sub:GatekeeperTest.ConsumeEvent(request/cache.rebuild:rbac)",
		});
		ASSERT_NO_THROW(identity.store());

		const datastore::Collection collection({
			.name = "name:GatekeeperTest.ConsumeEvent(request/cache.rebuild:rbac)",
		});
		ASSERT_NO_THROW(collection.store());
		ASSERT_NO_THROW(collection.add(identity.id()));

		const datastore::Role role({
			.name = "name:GatekeeperTest.ConsumeEvent(request/cache.rebuild:rbac)",
			.permissions =
				{
					{"permissions[0]:GatekeeperTest.ConsumeEvent(request/cache.rebuild:rbac)"},
				},
		});
		ASSERT_NO_THROW(role.store());

		const datastore::RbacPolicy policy({
			.name = "name:GatekeeperTest.ConsumeEvent(request/cache.rebuild:rbac)",
		});
		ASSERT_NO_THROW(policy.store());

		const datastore::RbacPolicy::Rule rule({
			.attrs  = R"({"key": "value"})",
			.roleId = role.id(),
		});
		ASSERT_NO_THROW(policy.addRule(rule));

		ASSERT_NO_THROW(policy.addCollection(collection.id()));

		const auto identityIds = policy.identities(true);
		EXPECT_EQ(1, identityIds.size());

		// Ensure cache is clear
		{
			for (const auto &id : identityIds) {
				for (const auto &perm : role.permissions()) {
					const datastore::RbacPolicy::Cache cache({
						.identity   = id,
						.permission = perm,
						.policy     = policy.id(),
						.rule       = rule,
					});

					ASSERT_NO_THROW(cache.discard());
				}
			}
		}

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::ConsumeEventResponse          response;

		gk::v1::RebuildRbacCacheEventPayload payload;
		payload.add_ids(policy.id());

		gk::v1::Event request;
		request.set_name("request/cache.rebuild:rbac");

		auto any = request.mutable_payload();
		any->PackFrom(payload);

		auto reactor = service.ConsumeEvent(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);

		// Expect cache to be rebuilt
		{
			for (const auto &id : identityIds) {
				for (const auto &perm : role.permissions()) {
					const auto policies = datastore::RbacPolicy::Cache::check(id, perm);
					ASSERT_EQ(1, policies.size());
					EXPECT_EQ(policy.id(), policies[0].id);
					EXPECT_EQ(*rule.attrs, policies[0].attrs);
				}
			}
		}
	}
}
