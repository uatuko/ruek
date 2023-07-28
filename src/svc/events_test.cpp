#include <grpcpp/test/default_reactor_test_peer.h>
#include <gtest/gtest.h>

#include "datastore/access-policies.h"
#include "datastore/collections.h"
#include "datastore/identities.h"
#include "datastore/permissions.h"
#include "datastore/rbac-policies.h"
#include "datastore/roles.h"
#include "datastore/testing.h"

#include "events.h"

class svc_EventsTest : public testing::Test {
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

TEST_F(svc_EventsTest, Process_cache_rebuild) {
	svc::Events svc;

	// Success: request/cache.rebuild:access
	{
		const datastore::Identity identity({
			.sub = "sub:svc_EventsTest.Process(request/cache.rebuild:access)",
		});
		ASSERT_NO_THROW(identity.store());

		const datastore::Collection collection({
			.name = "name:svc_EventsTest.Process(request/cache.rebuild:access)",
		});
		ASSERT_NO_THROW(collection.store());
		ASSERT_NO_THROW(collection.add(identity.id()));

		const datastore::AccessPolicy policy({
			.name = "name:svc_EventsTest.Process(request/cache.rebuild:access)",
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
		gk::v1::EventsProcessResponse         response;

		gk::v1::RebuildAccessCacheEventPayload payload;
		payload.add_ids(policy.id());

		gk::v1::EventsProcessRequest request;

		auto *event = request.mutable_event();
		event->set_name("request/cache.rebuild:access");

		auto any = event->mutable_payload();
		any->PackFrom(payload);

		auto reactor = svc.Process(&ctx, &request, &response);
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
			.sub = "sub:svc_EventsTest.Process(request/cache.rebuild:rbac)",
		});
		ASSERT_NO_THROW(identity.store());

		const datastore::Collection collection({
			.name = "name:svc_EventsTest.Process(request/cache.rebuild:rbac)",
		});
		ASSERT_NO_THROW(collection.store());
		ASSERT_NO_THROW(collection.add(identity.id()));

		datastore::Permission permission({
			.id = "permissions[0].id:svc_EventsTest.Process(request/cache.rebuild:rbac)",
		});
		ASSERT_NO_THROW(permission.store());

		const datastore::Role role({
			.name = "name:GatekeeperTest.ConsumeEvent(request/cache.rebuild:rbac)",
		});
		ASSERT_NO_THROW(role.store());
		ASSERT_NO_THROW(role.addPermission(permission.id()));

		const datastore::RbacPolicy policy({
			.name = "name:svc_EventsTest.Process(request/cache.rebuild:rbac)",
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
				for (const auto &perm : datastore::RetrieveRolePermissions(role.id())) {
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
		gk::v1::EventsProcessResponse         response;

		gk::v1::RebuildRbacCacheEventPayload payload;
		payload.add_ids(policy.id());

		gk::v1::EventsProcessRequest request;

		auto *event = request.mutable_event();
		event->set_name("request/cache.rebuild:rbac");

		auto any = event->mutable_payload();
		any->PackFrom(payload);

		auto reactor = svc.Process(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);

		// Expect cache to be rebuilt
		{
			for (const auto &id : identityIds) {
				for (const auto &perm : datastore::RetrieveRolePermissions(role.id())) {
					const auto policies = datastore::RbacPolicy::Cache::check(id, perm);
					ASSERT_EQ(1, policies.size());
					EXPECT_EQ(policy.id(), policies[0].id);
					EXPECT_EQ(*rule.attrs, policies[0].attrs);
				}
			}
		}
	}
}
