#include <grpcpp/test/default_reactor_test_peer.h>
#include <gtest/gtest.h>

#include "datastore/access-policies.h"
#include "datastore/collections.h"
#include "datastore/identities.h"
#include "datastore/testing.h"

#include "access.h"

class svc_AccessTest : public testing::Test {
protected:
	static void SetUpTestSuite() {
		datastore::testing::setup();

		// Clear data
		datastore::pg::exec("truncate table \"access-policies\" cascade;");
		datastore::pg::exec("truncate table collections cascade;");
		datastore::pg::exec("truncate table identities cascade;");
		datastore::redis::conn().cmd("flushall");
	}

	static void TearDownTestSuite() { datastore::testing::teardown(); }
};

TEST_F(svc_AccessTest, AddPolicyCollection) {
	svc::Access svc;

	// Success: add collection
	{
		const datastore::Collection collection({
			.name = "name:svc_AccessTest.AddPolicyCollection",
		});
		ASSERT_NO_THROW(collection.store());

		const datastore::AccessPolicy policy({
			.name = "name:svc_AccessTest.AddPolicyCollection",
		});
		ASSERT_NO_THROW(policy.store());
		EXPECT_EQ(0, policy.collections().size());

		grpc::CallbackServerContext               ctx;
		grpc::testing::DefaultReactorTestPeer     peer(&ctx);
		gk::v1::AddAccessPolicyCollectionResponse response;

		gk::v1::AddAccessPolicyCollectionRequest request;
		request.set_policy_id(policy.id());
		request.set_collection_id(collection.id());

		auto reactor = svc.AddPolicyCollection(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);

		const auto ids = policy.collections();
		ASSERT_EQ(1, ids.size());
		EXPECT_EQ(collection.id(), *ids.cbegin());
	}
}

TEST_F(svc_AccessTest, AddPolicyIdentity) {
	svc::Access svc;

	// Success: add identity
	{
		const datastore::Identity identity({
			.sub = "sub:svc_AccessTest.AddPolicyIdentity",
		});
		ASSERT_NO_THROW(identity.store());

		const datastore::AccessPolicy policy({
			.name = "name:svc_AccessTest.AddPolicyIdentity",
		});
		ASSERT_NO_THROW(policy.store());
		EXPECT_EQ(0, policy.identities().size());

		grpc::CallbackServerContext             ctx;
		grpc::testing::DefaultReactorTestPeer   peer(&ctx);
		gk::v1::AddAccessPolicyIdentityResponse response;

		gk::v1::AddAccessPolicyIdentityRequest request;
		request.set_policy_id(policy.id());
		request.set_identity_id(identity.id());

		auto reactor = svc.AddPolicyIdentity(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);

		const auto ids = policy.identities();
		ASSERT_EQ(1, ids.size());
		EXPECT_EQ(identity.id(), *ids.cbegin());
	}
}

TEST_F(svc_AccessTest, Check) {
	svc::Access svc;

	// Success: returns policy when found
	{
		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::CheckAccessResponse           response;

		const datastore::Identity identity({
			.sub = "sub:svc_AccessTest.Check",
		});
		ASSERT_NO_THROW(identity.store());

		const datastore::AccessPolicy policy({
			.name = "name:svc_AccessTest.Check",
		});
		ASSERT_NO_THROW(policy.store());

		const std::string resource = "resource/svc_AccessTest.Check";

		const datastore::AccessPolicy::Cache cache({
			.identity = identity.id(),
			.policy   = policy.id(),
			.rule     = {.resource = resource},
		});
		ASSERT_NO_THROW(cache.store());

		gk::v1::CheckAccessRequest request;
		request.set_resource(resource);
		request.set_identity_id(identity.id());

		auto reactor = svc.Check(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);
		EXPECT_EQ(1, response.policies().size());
	}
}

TEST_F(svc_AccessTest, CreatePolicy) {
	svc::Access svc;

	// Success: create access policy
	{
		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::AccessPolicy                  response;

		gk::v1::CreateAccessPolicyRequest request;
		request.set_name("name:svc_AccessTest.CreatePolicy");

		auto reactor = svc.CreatePolicy(&ctx, &request, &response);
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
		request.set_id("id:svc_AccessTest.CreatePolicy-with_id");
		request.set_name("name:svc_AccessTest.CreatePolicy-with_id");

		auto reactor = svc.CreatePolicy(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);
		EXPECT_EQ(request.id(), response.id());
		EXPECT_EQ(request.name(), response.name());
	}

	// Error: duplicate `id`
	{
		const datastore::AccessPolicy policy({
			.name = "name:svc_AccessTest.CreatePolicy-duplicate_id",
		});
		ASSERT_NO_THROW(policy.store());

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::AccessPolicy                  response;

		gk::v1::CreateAccessPolicyRequest request;
		request.set_id(policy.id());
		request.set_name("name:svc_AccessTest.CreatePolicy-duplicate_id");

		auto reactor = svc.CreatePolicy(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_EQ(grpc::StatusCode::ALREADY_EXISTS, peer.test_status().error_code());
		EXPECT_EQ("Duplicate policy id", peer.test_status().error_message());
	}

	// Success: create access policy with identity and resource
	{
		const datastore::Identity identity({
			.sub = "sub:svc_AccessTest.CreatePolicy-with_identity_and_resource",
		});
		ASSERT_NO_THROW(identity.store());

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::AccessPolicy                  response;

		gk::v1::CreateAccessPolicyRequest request;
		request.set_name("name:svc_AccessTest.CreatePolicy-with_identity_and_resource");
		request.add_identity_ids(identity.id());

		auto rule = request.add_rules();
		rule->set_resource("resource/svc_AccessTest.CreatePolicy-with_identity_and_resource");

		// Expect no access before request
		{
			const auto policies =
				datastore::AccessPolicy::Cache::check(identity.id(), rule->resource());
			EXPECT_EQ(0, policies.size());
		}

		// Create access policy
		auto reactor = svc.CreatePolicy(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);

		// Expect to find single policy when checking access
		{
			const auto policies =
				datastore::AccessPolicy::Cache::check(identity.id(), rule->resource());
			EXPECT_EQ(1, policies.size());
		}
	}

	// Success: create access policy with collection and resource
	{
		const datastore::Identity identity({
			.id  = "id:svc_AccessTest.CreatePolicy-with_collection_and_resource",
			.sub = "sub:svc_AccessTest.CreatePolicy-with_collection_and_resource",
		});
		ASSERT_NO_THROW(identity.store());

		const datastore::Collection collection({
			.id   = "id:svc_AccessTest.CreatePolicy-with_collection_and_resource",
			.name = "name:svc_AccessTest.CreatePolicy-with_collection_and_resource",
		});
		ASSERT_NO_THROW(collection.store());
		ASSERT_NO_THROW(collection.add(identity.id()));

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::AccessPolicy                  response;

		gk::v1::CreateAccessPolicyRequest request;
		request.set_name("name:svc_AccessTest.CreatePolicy-with_collection_and_resource");
		request.add_collection_ids(collection.id());

		auto rule = request.add_rules();
		rule->set_resource("resource/svc_AccessTest.CreatePolicy-with_collection_and_resource");

		// Expect no access before request
		{
			const auto policies =
				datastore::AccessPolicy::Cache::check(identity.id(), rule->resource());
			EXPECT_EQ(0, policies.size());
		}

		// Create access policy
		auto reactor = svc.CreatePolicy(&ctx, &request, &response);
		EXPECT_TRUE(peer.test_status_set());
		EXPECT_TRUE(peer.test_status().ok());
		EXPECT_EQ(peer.reactor(), reactor);

		// Expect to find single policy when checking access
		{
			const auto policies =
				datastore::AccessPolicy::Cache::check(identity.id(), rule->resource());
			EXPECT_EQ(1, policies.size());
		}
	}
}

TEST_F(svc_AccessTest, RetrievePolicy) {
	svc::Access svc;

	// Success: retrieve data
	{
		const datastore::Identities identities({
			{{.sub = "sub:svc_AccessTest.RetrievePolicy[0]"}},
			{{.sub = "sub:svc_AccessTest.RetrievePolicy[1]"}},
		});

		for (const auto &idn : identities) {
			ASSERT_NO_THROW(idn.store());
		}

		const datastore::Collection collection({
			.name = "name:svc_AccessTest.RetrievePolicy",
		});
		ASSERT_NO_THROW(collection.store());
		ASSERT_NO_THROW(collection.add(identities[0].id()));

		const datastore::AccessPolicy policy({
			.name = "name:svc_AccessTest.RetrievePolicy",
			.rules =
				{
					{
						.attrs    = R"({"key":"value"})",
						.resource = "resource/svc_AccessTest.RetrievePolicy",
					},
				},
		});
		ASSERT_NO_THROW(policy.store());
		ASSERT_NO_THROW(policy.addCollection(collection.id()));
		ASSERT_NO_THROW(policy.addIdentity(identities[1].id()));

		grpc::CallbackServerContext           ctx;
		grpc::testing::DefaultReactorTestPeer peer(&ctx);
		gk::v1::AccessPolicy                  response;

		gk::v1::RetrieveAccessPolicyRequest request;
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

		ASSERT_EQ(1, response.rules_size());
		EXPECT_EQ(policy.rules().cbegin()->resource, response.rules(0).resource());

		std::string attrs;
		google::protobuf::util::MessageToJsonString(response.rules(0).attrs(), &attrs);
		EXPECT_EQ(policy.rules().cbegin()->attrs, attrs);
	}
}
