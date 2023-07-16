#include "gatekeeper.h"

#include "err/errors.h"

#include "mappers.h"

namespace service {
// Checks
grpc::ServerUnaryReactor *Gatekeeper::CheckAccess(
	grpc::CallbackServerContext *context, const gk::v1::CheckAccessRequest *request,
	gk::v1::CheckAccessResponse *response) {
	auto *reactor = context->DefaultReactor();

	if (request->has_identity_sub()) {
		// TODO: implement checking access by sub
		reactor->Finish(grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Not implemented"));
		return reactor;
	}

	try {
		const auto policies =
			datastore::AccessPolicy::Cache::check(request->identity_id(), request->resource());
		map(policies, response);
	} catch (...) {
		reactor->Finish(grpc::Status(grpc::StatusCode::UNAVAILABLE, "Failed to check access"));
		return reactor;
	}

	reactor->Finish(grpc::Status::OK);
	return reactor;
}

grpc::ServerUnaryReactor *Gatekeeper::CheckRbac(
	grpc::CallbackServerContext *context, const gk::v1::CheckRbacRequest *request,
	gk::v1::CheckRbacResponse *response) {
	auto *reactor = context->DefaultReactor();

	if (request->has_identity_sub()) {
		// TODO: implement checking access by sub
		reactor->Finish(grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Not implemented"));
		return reactor;
	}

	try {
		const auto policies =
			datastore::RbacPolicy::Cache::check(request->identity_id(), request->permission());
		map(policies, response);
	} catch (...) {
		reactor->Finish(grpc::Status(grpc::StatusCode::UNAVAILABLE, "Failed to check rbac"));
		return reactor;
	}

	reactor->Finish(grpc::Status::OK);
	return reactor;
}

// Events
grpc::ServerUnaryReactor *Gatekeeper::ConsumeEvent(
	grpc::CallbackServerContext *context, const gk::v1::Event *request,
	gk::v1::ConsumeEventResponse *response) {
	auto *reactor = context->DefaultReactor();

	if (request->name() == "request/cache.rebuild:access") {
		gk::v1::RebuildAccessCacheEventPayload payload;
		if (!request->payload().UnpackTo(&payload)) {
			reactor->Finish(grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Invalid payload"));
			return reactor;
		}

		for (const auto &id : payload.ids()) {
			const auto policy = datastore::RetrieveAccessPolicy(id);
			for (const auto &identity : policy.identities(true)) {
				for (const auto &rule : policy.rules()) {
					const datastore::AccessPolicy::Cache cache({
						.identity = identity,
						.policy   = policy.id(),
						.rule     = rule,
					});

					cache.store();
				}
			}
		}
	} else if (request->name() == "request/cache.rebuild:rbac") {
		gk::v1::RebuildRbacCacheEventPayload payload;
		if (!request->payload().UnpackTo(&payload)) {
			reactor->Finish(grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Invalid payload"));
			return reactor;
		}

		for (const auto &id : payload.ids()) {
			const auto policy = datastore::RetrieveRbacPolicy(id);
			for (const auto &identity : policy.identities(true)) {
				for (const auto &rule : policy.rules()) {
					const auto role = datastore::RetrieveRole(rule.roleId);
					for (const auto &perm : role.permissions()) {
						const datastore::RbacPolicy::Cache cache({
							.identity   = identity,
							.permission = perm,
							.policy     = policy.id(),
							.rule       = rule,
						});

						cache.store();
					}
				}
			}
		}
	} else {
		reactor->Finish(grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Unknown event"));
		return reactor;
	}

	reactor->Finish(grpc::Status::OK);
	return reactor;
}
} // namespace service
