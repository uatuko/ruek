#include <grpcpp/test/default_reactor_test_peer.h>
#include <gtest/gtest.h>

#include "datastore/access-policies.h"
#include "datastore/collections.h"
#include "datastore/identities.h"
#include "datastore/rbac-policies.h"
#include "datastore/roles.h"
#include "datastore/testing.h"

#include "collections.h"

class svc_CollectionsTest : public testing::Test {
protected:
	static void SetUpTestSuite() {
		datastore::testing::setup();

		// Clear data
		datastore::pg::exec("truncate table collections cascade;");
		datastore::pg::exec("truncate table identities cascade;");
	}

	static void TearDownTestSuite() { datastore::testing::teardown(); }
};

TEST_F(svc_CollectionsTest, AddMember) {
	const datastore::Collection collection({.name = "name:svc_CollectionsTest.AddMember"});
	ASSERT_NO_THROW(collection.store());

	svc::Collections svc;

	// Success: add collection member
	{
		datastore::Identity identity({.sub = "sub:svc_CollectionsTest.AddMember"});
		ASSERT_NO_THROW(identity.store());

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::CollectionsAddMemberResponse  response;

		gk::v1::CollectionsAddMemberRequest request;
		request.set_collection_id(collection.id());
		request.set_identity_id(identity.id());

		auto reactor = svc.AddMember(&ctx, &request, &response);
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

	// Success: adding collection member should propagate to access policy cache
	{
		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::CollectionsAddMemberResponse  response;

		const datastore::Identity identity({
			.id  = "id:svc_CollectionsTest.AddMember-access",
			.sub = "sub:svc_CollectionsTest.AddMember-access",
		});
		ASSERT_NO_THROW(identity.store());

		const datastore::Collection collection({
			.id   = "id:svc_CollectionsTest.AddMember-access",
			.name = "name:svc_CollectionsTest.AddMember-access",
		});
		ASSERT_NO_THROW(collection.store());

		gk::v1::CollectionsAddMemberRequest request;
		request.set_collection_id(collection.id());
		request.set_identity_id(identity.id());

		const datastore::AccessPolicy::Rule rule({
			.attrs    = R"({"key": "value"})",
			.resource = "resource/svc_CollectionsTest.AddMember-access",
		});

		std::set<datastore::AccessPolicy::Rule> rules;
		rules.insert(rule);

		const datastore::AccessPolicy policy({
			.name  = "name:svc_CollectionsTest.AddMember",
			.rules = rules,
		});
		ASSERT_NO_THROW(policy.store());
		ASSERT_NO_THROW(policy.addCollection(collection.id()));

		// expect no access before request
		{
			const auto policies =
				datastore::AccessPolicy::Cache::check(identity.id(), rule.resource);
			EXPECT_EQ(0, policies.size());
		}

		auto reactor = svc.AddMember(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);

		// expect to find single policy when checking access
		{
			const auto policies =
				datastore::AccessPolicy::Cache::check(identity.id(), rule.resource);
			EXPECT_EQ(1, policies.size());
		}
	}

	// Success: adding collection member should propagate to rbac policy cache
	{
		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::CollectionsAddMemberResponse  response;

		const datastore::Identity identity({
			.id  = "id:svc_CollectionsTest.AddMember-rbac",
			.sub = "sub:svc_CollectionsTest.AddMember-rbac",
		});
		ASSERT_NO_THROW(identity.store());

		const datastore::Collection collection({
			.id   = "id:svc_CollectionsTest.AddMember-rbac",
			.name = "name:svc_CollectionsTest.AddMember-rbac",
		});
		ASSERT_NO_THROW(collection.store());

		gk::v1::CollectionsAddMemberRequest request;
		request.set_collection_id(collection.id());
		request.set_identity_id(identity.id());

		const std::string perm = "permissions[0]:svc_CollectionsTest.AddMember-rbac";

		const datastore::RbacPolicy policy({
			.name = "name:svc_CollectionsTest.AddMember-rbac",
		});
		ASSERT_NO_THROW(policy.store());

		const datastore::Role role({
			.name = "name:svc_CollectionsTest.AddMember-rbac",
			.permissions =
				{
					perm,
				},
		});
		ASSERT_NO_THROW(role.store());

		auto rule = datastore::RbacPolicy::Rule({.roleId = role.id()});

		ASSERT_NO_THROW(policy.addRule(rule));
		ASSERT_NO_THROW(policy.store());
		ASSERT_NO_THROW(policy.addCollection(collection.id()));

		// expect no access before request
		{
			const auto policies = datastore::RbacPolicy::Cache::check(identity.id(), perm);
			EXPECT_EQ(0, policies.size());
		}

		// add user
		auto reactor = svc.AddMember(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);

		// expect to find single policy when checking rbac
		{
			const auto policies = datastore::RbacPolicy::Cache::check(identity.id(), perm);
			EXPECT_EQ(1, policies.size());
		}
	}
}

TEST_F(svc_CollectionsTest, Create) {
	svc::Collections svc;

	// Success: create collection
	{
		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Collection                    response;

		gk::v1::CollectionsCreateRequest request;
		request.set_name("name:svc_CollectionsTest.Create");

		auto reactor = svc.Create(&ctx, &request, &response);
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

		gk::v1::CollectionsCreateRequest request;
		request.set_id("id:svc_CollectionsTest.Create-with_id");
		request.set_name("name:svc_CollectionsTest.Create-with_id");

		auto reactor = svc.Create(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);
		EXPECT_EQ(request.id(), response.id());
		EXPECT_EQ(request.name(), response.name());
	}

	// Error: duplicate `id`
	{
		const datastore::Collection collection({
			.name = "name:svc_CollectionsTest.Create-duplicate_id",
		});
		ASSERT_NO_THROW(collection.store());

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Collection                    response;

		gk::v1::CollectionsCreateRequest request;
		request.set_id(collection.id());
		request.set_name("name:svc_CollectionsTest.Create-duplicate_id");

		auto reactor = svc.Create(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_EQ(grpc::StatusCode::ALREADY_EXISTS, peer.test_status().error_code());
		EXPECT_EQ("Duplicate collection id", peer.test_status().error_message());
	}
}

TEST_F(svc_CollectionsTest, ListMembers) {
	datastore::Collection collection({
		.name = "name:svc_CollectionsTest.ListMembers",
	});
	ASSERT_NO_THROW(collection.store());

	svc::Collections svc;

	// Success: list collection members
	{
		std::array<datastore::Identity, 2> identities = {
			datastore::Identity({.sub = "sub:svc_CollectionsTest.ListMembers-1"}),
			datastore::Identity({.sub = "sub:svc_CollectionsTest.ListMembers-2"}),
		};

		for (const auto &idn : identities) {
			ASSERT_NO_THROW(idn.store());
		}

		ASSERT_NO_THROW(collection.add(identities[0].id()));

		grpc::CallbackServerContext            ctx;
		grpc::testing::DefaultReactorTestPeer  peer(&ctx);
		gk::v1::CollectionsListMembersResponse response;

		gk::v1::CollectionsListMembersRequest request;
		request.set_id(collection.id());

		auto reactor = svc.ListMembers(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);

		EXPECT_FALSE(response.has_meta());

		ASSERT_EQ(1, response.data_size());
		EXPECT_EQ(identities[0].id(), response.data(0).id());
	}
}

TEST_F(svc_CollectionsTest, RemoveMember) {
	datastore::Collection collection({
		.name = "name:svc_CollectionsTest.RemoveMember",
	});
	ASSERT_NO_THROW(collection.store());

	svc::Collections svc;

	// Success: remove collection member
	{
		datastore::Identity identity({
			.sub = "sub:svc_CollectionsTest.RemoveMember",
		});
		ASSERT_NO_THROW(identity.store());
		ASSERT_NO_THROW(collection.add(identity.id()));

		grpc::CallbackServerContext             ctx;
		grpc::testing::DefaultReactorTestPeer   peer(&ctx);
		gk::v1::CollectionsRemoveMemberResponse response;

		gk::v1::CollectionsRemoveMemberRequest request;
		request.set_collection_id(collection.id());
		request.set_identity_id(identity.id());

		auto reactor = svc.RemoveMember(&ctx, &request, &response);
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

TEST_F(svc_CollectionsTest, Retrieve) {
	svc::Collections svc;

	// Success: retrieve collection by id
	{
		const datastore::Collection collection({
			.id   = "id:svc_CollectionsTest.Retrieve",
			.name = "name:svc_CollectionsTest.Retrieve",
		});
		ASSERT_NO_THROW(collection.store());

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Collection                    response;

		gk::v1::CollectionsRetrieveRequest request;
		request.set_id(collection.id());

		auto reactor = svc.Retrieve(&ctx, &request, &response);
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

		gk::v1::CollectionsRetrieveRequest request;
		request.set_id("id:svc_CollectionsTest.Retrieve-not_found");

		auto reactor = svc.Retrieve(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_EQ(grpc::StatusCode::NOT_FOUND, peer.test_status().error_code());
		EXPECT_EQ("Document not found", peer.test_status().error_message());
	}
}

TEST_F(svc_CollectionsTest, Update) {
	svc::Collections svc;

	// Success: update collection name
	{
		const datastore::Collection collection({
			.id   = "id:svc_CollectionsTest.Update",
			.name = "name:svc_CollectionsTest.Update",
		});
		ASSERT_NO_THROW(collection.store());

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Collection                    response;

		gk::v1::CollectionsUpdateRequest request;
		request.set_id(collection.id());
		request.set_name("name:svc_CollectionsTest.Update-edit");

		auto reactor = svc.Update(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);

		EXPECT_EQ(request.id(), response.id());
		EXPECT_EQ(request.name(), response.name());

		const auto updated = datastore::RetrieveCollection(collection.id());
		EXPECT_EQ(request.name(), updated.name());
		EXPECT_EQ(1, updated.rev());
	}

	// Error: no fields to update
	{
		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Collection                    response;

		gk::v1::CollectionsUpdateRequest request;
		request.set_id("id:svc_CollectionsTest.Update-no_changes");

		auto reactor = svc.Update(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_EQ(grpc::StatusCode::INTERNAL, peer.test_status().error_code());
		EXPECT_EQ("No fields to update", peer.test_status().error_message());
	}

	// Error: collection not found
	{
		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::Collection                    response;

		gk::v1::CollectionsUpdateRequest request;
		request.set_id("id:svc_CollectionsTest.Update-not_found");
		request.set_name("name:svc_CollectionsTest.Update-not_found");

		auto reactor = svc.Update(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_EQ(grpc::StatusCode::NOT_FOUND, peer.test_status().error_code());
		EXPECT_EQ("Document not found", peer.test_status().error_message());
	}
}
