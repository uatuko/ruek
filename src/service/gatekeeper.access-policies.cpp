#include "gatekeeper.h"

#include "err/errors.h"

#include "mappers.h"

namespace service {
grpc::ServerUnaryReactor *Gatekeeper::CreateAccessPolicy(
	grpc::CallbackServerContext *context, const gk::v1::CreateAccessPolicyRequest *request,
	gk::v1::AccessPolicy *response) {
	auto *reactor = context->DefaultReactor();

	if (request->has_id()) {
		try {
			auto policy = datastore::RetrieveAccessPolicy(request->id());
			reactor->Finish(grpc::Status(grpc::StatusCode::ALREADY_EXISTS, "Duplicate policy id"));
			return reactor;
		} catch (const err::DatastoreAccessPolicyNotFound &) {
			// Policy with an `id` matching the request `id` doesn't exist, we can continue with
			// creating a new one.
		} catch (...) {
			reactor->Finish(grpc::Status(grpc::StatusCode::UNAVAILABLE, "Failed to retrieve data"));
			return reactor;
		}
	}

	auto policy = map(request);
	try {
		policy.store();
	} catch (...) {
		reactor->Finish(grpc::Status(grpc::StatusCode::UNAVAILABLE, "Failed to store data"));
		return reactor;
	}

	for (const auto &id : request->collection_ids()) {
		policy.addCollection(id);
	}

	for (const auto &id : request->identity_ids()) {
		policy.addIdentity(id);
	}

	map(policy, response);

	reactor->Finish(grpc::Status::OK);
	return reactor;
}

grpc::ServerUnaryReactor *Gatekeeper::RetrieveAccessPolicy(
	grpc::CallbackServerContext *context, const gk::v1::RetrieveAccessPolicyRequest *request,
	gk::v1::AccessPolicy *response) {
	auto *reactor = context->DefaultReactor();

	auto policy = datastore::RetrieveAccessPolicy(request->id());
	map(policy, response);

	for (const auto &id : policy.collections()) {
		response->add_collection_ids(id);
	}

	for (const auto &id : policy.identities()) {
		response->add_identity_ids(id);
	}

	reactor->Finish(grpc::Status::OK);
	return reactor;
}
} // namespace service
