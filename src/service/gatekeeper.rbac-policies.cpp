#include "gatekeeper.h"

#include "err/errors.h"

#include "mappers.h"

namespace service {
grpc::ServerUnaryReactor *Gatekeeper::CreateRbacPolicy(
	grpc::CallbackServerContext *context, const gk::v1::CreateRbacPolicyRequest *request,
	gk::v1::RbacPolicy *response) {
	auto *reactor = context->DefaultReactor();

	if (request->has_id()) {
		try {
			auto policy = datastore::RetrieveRbacPolicy(request->id());
			reactor->Finish(grpc::Status(grpc::StatusCode::ALREADY_EXISTS, "Duplicate policy id"));
			return reactor;
		} catch (const err::DatastoreRbacPolicyNotFound &) {
			// Policy with an `id` matching the request `id` doesn't exist, we can continue with
			// creating a new one.
		} catch (const std::exception &e) {
			reactor->Finish(grpc::Status(grpc::StatusCode::UNAVAILABLE, "Failed to retrieve data"));
			return reactor;
		}
	}

	auto policy = map(request);
	try {
		// Store the policy
		policy.store();

		// Add rules
		if (request->rules().size() > 0) {
			for (const auto &rule : request->rules()) {
				policy.addRule({.roleId = rule.role_id()});
			}
		}

		// Add principals
		for (const auto &id : request->collection_ids()) {
			policy.addCollection(id);
		}

		for (const auto &id : request->identity_ids()) {
			policy.addIdentity(id);
		}
	} catch (std::exception &e) {
		reactor->Finish(grpc::Status(grpc::StatusCode::UNAVAILABLE, "Failed to store data"));
		return reactor;
	}

	map(policy, response);

	reactor->Finish(grpc::Status::OK);
	return reactor;
}
} // namespace service
