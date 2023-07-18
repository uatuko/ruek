#include "gatekeeper.h"

#include <grpcpp/test/default_reactor_test_peer.h>
#include <gtest/gtest.h>

#include "datastore/access-policies.h"
#include "datastore/collections.h"
#include "datastore/identities.h"
#include "datastore/testing.h"

class GatekeeperAccessPoliciesTest : public testing::Test {
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

TEST_F(GatekeeperAccessPoliciesTest, CreateAccessPolicy) {
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

TEST_F(GatekeeperAccessPoliciesTest, RetrieveAccessPolicy) {
	service::Gatekeeper service;

	// Success: retrieve data
	{
		const datastore::Identities identities({
			{{.sub = "sub:GatekeeperAccessPoliciesTest.RetrieveAccessPolicy[0]"}},
			{{.sub = "sub:GatekeeperAccessPoliciesTest.RetrieveAccessPolicy[1]"}},
		});

		for (const auto &idn : identities) {
			ASSERT_NO_THROW(idn.store());
		}

		const datastore::Collection collection({
			.name = "name:GatekeeperAccessPoliciesTest.RetrieveAccessPolicy",
		});
		ASSERT_NO_THROW(collection.store());
		ASSERT_NO_THROW(collection.add(identities[0].id()));

		const datastore::AccessPolicy policy({
			.name = "name:GatekeeperAccessPoliciesTest.RetrieveAccessPolicy",
			.rules =
				{
					{
						.attrs    = R"({"key":"value"})",
						.resource = "resource/GatekeeperAccessPoliciesTest.RetrieveAccessPolicy",
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

		auto reactor = service.RetrieveAccessPolicy(&ctx, &request, &response);
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
