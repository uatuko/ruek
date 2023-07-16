#include <grpcpp/test/default_reactor_test_peer.h>
#include <gtest/gtest.h>

#include "datastore/access-policies.h"
#include "datastore/collections.h"
#include "datastore/identities.h"
#include "datastore/rbac-policies.h"
#include "datastore/roles.h"
#include "datastore/testing.h"

#include "gatekeeper.h"

class GatekeeperTest : public ::testing::Test {
protected:
	static void SetUpTestSuite() {
		datastore::testing::setup();

		// Clear data
		datastore::pg::exec("truncate table \"access-policies\" cascade;");
		datastore::pg::exec("truncate table collections cascade;");
		datastore::pg::exec("truncate table identities cascade;");
		datastore::redis::conn().cmd("flushall");
	}

	void SetUp() {
		// Clear data before each test
		datastore::pg::exec("delete from \"access-policies\" cascade;");
		datastore::pg::exec("delete from collections cascade;");
		datastore::pg::exec("delete from identities cascade;");
		datastore::pg::exec("delete from \"rbac-policies\" cascade;");
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

// Access Policies
TEST_F(GatekeeperTest, CreateAccessPolicy) {
	service::Gatekeeper service;

	// Success: create access policy
	{
		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::AccessPolicy                  response;

		gk::v1::CreateAccessPolicyRequest request;
		request.set_name("name:GatekeeperTest.CreateAccessPolicy");

		auto reactor = service.CreateAccessPolicy(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);
		EXPECT_EQ(request.name(), response.name());
		EXPECT_FALSE(response.id().empty());
	}

	// Success: create access policy with `id`
	{
		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::AccessPolicy                  response;

		gk::v1::CreateAccessPolicyRequest request;
		request.set_id("id:GatekeeperTest.CreateAccessPolicy");
		request.set_name("name:GatekeeperTest.CreateAccessPolicy");

		auto reactor = service.CreateAccessPolicy(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);
		EXPECT_EQ(request.id(), response.id());
		EXPECT_EQ(request.name(), response.name());
	}

	// Error: duplicate `id`
	{
		const datastore::AccessPolicy policy({.name = "name:GatekeeperTest.CreateAccessPolicy"});
		EXPECT_NO_THROW(policy.store());

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::AccessPolicy                  response;

		gk::v1::CreateAccessPolicyRequest request;
		request.set_id(policy.id());
		request.set_name("name:GatekeeperTest.CreateAccessPolicy-duplicate");

		auto reactor = service.CreateAccessPolicy(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_EQ(grpc::StatusCode::ALREADY_EXISTS, peer.test_status().error_code());
		EXPECT_EQ("Duplicate policy id", peer.test_status().error_message());
	}

	// Success: create access policy with an identity and resource
	{
		const datastore::Identity identity({
			.sub = "principal_sub:GatekeeperTest.CreateAccessPolicy",
		});
		ASSERT_NO_THROW(identity.store());

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::AccessPolicy                  response;

		gk::v1::CreateAccessPolicyRequest request;
		request.set_name("name:GatekeeperTest.CreateAccessPolicy");
		request.add_identity_ids(identity.id());

		auto rule = request.add_rules();
		rule->set_resource("resource_id:GatekeeperTest.CreateAccessPolicy");

		// expect no access before request
		{
			const auto policies =
				datastore::AccessPolicy::Cache::check(identity.id(), rule->resource());
			EXPECT_EQ(0, policies.size());
		}

		// create access policy
		auto reactor = service.CreateAccessPolicy(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);

		// expect to find single policy when checking access
		{
			const auto policies =
				datastore::AccessPolicy::Cache::check(identity.id(), rule->resource());
			EXPECT_EQ(1, policies.size());
		}
	}

	// Success: create an access policy for collection
	// all members of collection should be granted access
	{
		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::AccessPolicy                  response;

		const datastore::Identity   identity({
			  .id  = "identity_id:GatekeeperTest.CreateAccessPolicy",
			  .sub = "identity_sub:GatekeeperTest.CreateAccessPolicy",
        });
		const datastore::Collection collection({
			.id   = "collection_id:GatekeeperTest.CreateAccessPolicy",
			.name = "collection_name:GatekeeperTest.CreateAccessPolicy",
		});

		try {
			identity.store();
			collection.store();
			collection.add(identity.id());
		} catch (const std::exception &e) {
			FAIL() << e.what();
		}

		gk::v1::CreateAccessPolicyRequest request;
		request.set_name("name:GatekeeperTest.CreateAccessPolicy");
		request.add_collection_ids(collection.id());

		auto rule = request.add_rules();
		rule->set_resource("resource_id:GatekeeperTest.CreateAccessPolicy");

		// expect no access before request
		{
			const auto policies =
				datastore::AccessPolicy::Cache::check(identity.id(), rule->resource());
			EXPECT_EQ(0, policies.size());
		}

		// create access policy
		auto reactor = service.CreateAccessPolicy(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);

		// expect to find single policy when checking access
		{
			const auto policies =
				datastore::AccessPolicy::Cache::check(identity.id(), rule->resource());
			EXPECT_EQ(1, policies.size());
		}
	}

	// FIXME: nested collections
}

// Collections
TEST_F(GatekeeperTest, CreateCollection) {
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

TEST_F(GatekeeperTest, RetrieveCollection) {
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

TEST_F(GatekeeperTest, UpdateCollection) {
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

// Collections - members
TEST_F(GatekeeperTest, AddCollectionMember) {
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

TEST_F(GatekeeperTest, ListCollectionMembers) {
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

TEST_F(GatekeeperTest, RemoveCollectionMember) {
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

// Identities
TEST_F(GatekeeperTest, CreateIdentity) {
	service::Gatekeeper service;

	// Success: create identity
	{
		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Identity                      response;

		gk::v1::CreateIdentityRequest request;
		request.set_sub("sub:GatekeeperTest.CreateIdentity");

		auto reactor = service.CreateIdentity(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);
		EXPECT_EQ(request.sub(), response.sub());
		EXPECT_FALSE(response.id().empty());
		EXPECT_FALSE(response.has_attrs());
	}

	// Success: create identity with `id`
	{
		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Identity                      response;

		gk::v1::CreateIdentityRequest request;
		request.set_id("id:GatekeeperTest.CreateIdentity-with_id");
		request.set_sub("sub:GatekeeperTest.CreateIdentity-with_id");

		auto reactor = service.CreateIdentity(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);
		EXPECT_EQ(request.id(), response.id());
		EXPECT_EQ(request.sub(), response.sub());
		EXPECT_FALSE(response.has_attrs());
	}

	// Success: create identity with `attrs`
	{
		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Identity                      response;

		gk::v1::CreateIdentityRequest request;
		request.set_sub("sub:GatekeeperTest.CreateIdentity-with_attrs");

		const std::string attrs(R"({"foo":"bar"})");
		google::protobuf::util::JsonStringToMessage(attrs, request.mutable_attrs());

		auto reactor = service.CreateIdentity(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);
		EXPECT_EQ(request.sub(), response.sub());
		EXPECT_TRUE(response.has_attrs());

		std::string responseAttrs;
		google::protobuf::util::MessageToJsonString(response.attrs(), &responseAttrs);
		EXPECT_EQ(attrs, responseAttrs);
	}

	// Error: duplicate `id`
	{
		const datastore::Identity identity({
			.sub = "sub:GatekeeperTest.CreateIdentity-duplicate_id",
		});

		try {
			identity.store();
		} catch (const std::exception &e) {
			FAIL() << e.what();
		}

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Identity                      response;

		gk::v1::CreateIdentityRequest request;
		request.set_id(identity.id());
		request.set_sub("sub:GatekeeperTest.CreateIdentity-duplicate_id");

		auto reactor = service.CreateIdentity(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_EQ(grpc::StatusCode::ALREADY_EXISTS, peer.test_status().error_code());
		EXPECT_EQ("Duplicate identity id", peer.test_status().error_message());
	}

	// Error: duplicate `sub`
	{
		const datastore::Identity identity({
			.sub = "sub:GatekeeperTest.CreateIdentity-duplicate",
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

TEST_F(GatekeeperTest, RetrieveIdentity) {
	service::Gatekeeper service;

	// Success: retrieve identity by id
	{
		const datastore::Identity identity({
			.id  = "id:GatekeeperTest.RetrieveIdentity",
			.sub = "sub:GatekeeperTest.RetrieveIdentity",
		});
		ASSERT_NO_THROW(identity.store());

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Identity                      response;

		gk::v1::RetrieveIdentityRequest request;
		request.set_id(identity.id());

		auto reactor = service.RetrieveIdentity(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);
		EXPECT_EQ(identity.id(), response.id());
		EXPECT_EQ(identity.sub(), response.sub());
		EXPECT_FALSE(response.has_attrs());
	}

	// Success: retrieve identity by sub
	{
		const datastore::Identity identity({
			.id  = "id:GatekeeperTest.RetrieveIdentity-by_sub",
			.sub = "sub:GatekeeperTest.RetrieveIdentity-by_sub",
		});
		ASSERT_NO_THROW(identity.store());

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Identity                      response;

		gk::v1::RetrieveIdentityRequest request;
		request.set_sub(identity.sub());

		auto reactor = service.RetrieveIdentity(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);
		EXPECT_EQ(identity.id(), response.id());
		EXPECT_EQ(identity.sub(), response.sub());
		EXPECT_FALSE(response.has_attrs());
	}

	// Success: retrieve identity by id with attrs
	{
		const datastore::Identity identity({
			.attrs = R"({"flag":true})",
			.sub   = "sub:GatekeeperTest.RetrieveIdentity-with_attrs",
		});
		ASSERT_NO_THROW(identity.store());

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Identity                      response;

		gk::v1::RetrieveIdentityRequest request;
		request.set_id(identity.id());

		auto reactor = service.RetrieveIdentity(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);
		EXPECT_EQ(identity.id(), response.id());
		EXPECT_EQ(identity.sub(), response.sub());
		EXPECT_TRUE(response.has_attrs());

		std::string attrs;
		google::protobuf::util::MessageToJsonString(response.attrs(), &attrs);
		EXPECT_EQ(*identity.attrs(), attrs);
	}

	// Error: identity not found
	{
		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Identity                      response;

		gk::v1::RetrieveIdentityRequest request;
		request.set_id("id:GatekeeperTest.RetrieveIdentity-not_found");

		auto reactor = service.RetrieveIdentity(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_EQ(grpc::StatusCode::NOT_FOUND, peer.test_status().error_code());
		EXPECT_EQ("Document not found", peer.test_status().error_message());
	}
}

TEST_F(GatekeeperTest, UpdateIndentity) {
	service::Gatekeeper service;

	// Success: update identity sub
	{
		const datastore::Identity identity(
			{.id  = "id:GatekeeperTest.UpdateIdentity-sub",
			 .sub = "sub:GatekeeperTest.UpdateIdentity-sub"});
		ASSERT_NO_THROW(identity.store());

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Identity                      response;

		gk::v1::UpdateIdentityRequest request;
		request.set_id(identity.id());
		request.set_sub("sub:GatekeeperTest.UpdateIdentity-new");

		auto reactor = service.UpdateIdentity(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);

		EXPECT_EQ(request.id(), response.id());
		EXPECT_EQ(request.sub(), response.sub());
		EXPECT_FALSE(response.has_attrs());

		auto updatedIdentity = datastore::RetrieveIdentity(identity.id());
		EXPECT_EQ(request.sub(), updatedIdentity.sub());
		EXPECT_EQ(1, updatedIdentity.rev());
		EXPECT_FALSE(updatedIdentity.attrs());
	}

	// Success: update identity attrs
	{
		const datastore::Identity identity({.sub = "sub:GatekeeperTest.UpdateIdentity-attrs"});
		ASSERT_NO_THROW(identity.store());

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Identity                      response;

		gk::v1::UpdateIdentityRequest request;
		request.set_id(identity.id());
		{
			auto &attrs  = *request.mutable_attrs();
			auto &fields = *attrs.mutable_fields();

			{
				google::protobuf::Value v;
				v.set_bool_value(true);

				fields["flag"] = v;
			}

			{
				google::protobuf::Value v;
				v.set_string_value("Jane Doe");

				fields["name"] = v;
			}
		}

		auto reactor = service.UpdateIdentity(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);

		ASSERT_TRUE(response.has_attrs());
		auto &req = *request.mutable_attrs()->mutable_fields();
		auto &res = *response.mutable_attrs()->mutable_fields();

		EXPECT_EQ(req["flag"].bool_value(), res["flag"].bool_value());
		EXPECT_EQ(req["name"].string_value(), res["name"].string_value());

		{
			std::string_view qry = R"(
					select
						attrs->'flag' as flag,
						attrs->>'name' as name
					from identities
					where _id = $1::text;
				)";

			auto res = datastore::pg::exec(qry, identity.id());
			ASSERT_EQ(1, res.size());

			auto [flag, name] = res[0].as<bool, std::string>();
			EXPECT_EQ(req["flag"].bool_value(), flag);
			EXPECT_EQ(req["name"].string_value(), name);
		}
	}

	// Error: no fields to update
	{
		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Identity                      response;

		gk::v1::UpdateIdentityRequest request;
		request.set_id("id:GatekeeperTest.UpdateIdentity-no-updates");

		auto reactor = service.UpdateIdentity(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_EQ(grpc::StatusCode::INTERNAL, peer.test_status().error_code());
		EXPECT_EQ("No fields to update", peer.test_status().error_message());
	}

	// Error: identity not found
	{
		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Identity                      response;

		gk::v1::UpdateIdentityRequest request;
		request.set_id("id:GatekeeperTest.UpdateIdentity-not-found");
		request.set_sub("name:GatekeeperTest.UpdateIdentity-not-found");

		auto reactor = service.UpdateIdentity(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_EQ(grpc::StatusCode::NOT_FOUND, peer.test_status().error_code());
		EXPECT_EQ("Document not found", peer.test_status().error_message());
	}
}

// RBAC
TEST_F(GatekeeperTest, CreateRbacPolicy) {
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

// Roles
TEST_F(GatekeeperTest, CreateRole) {
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

TEST_F(GatekeeperTest, RetrieveRole) {
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
