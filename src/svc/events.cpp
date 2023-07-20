#include "events.h"

#include "datastore/access-policies.h"
#include "datastore/rbac-policies.h"
#include "datastore/roles.h"

namespace svc {
grpc::ServerUnaryReactor *Events::Process(
	grpc::CallbackServerContext *context, const gk::v1::EventsProcessRequest *request,
	gk::v1::EventsProcessResponse *response) {
	auto *reactor = context->DefaultReactor();

	auto event = request->event();
	if (event.name() == "request/cache.rebuild:access") {
		gk::v1::RebuildAccessCacheEventPayload payload;
		if (!event.payload().UnpackTo(&payload)) {
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
	} else if (event.name() == "request/cache.rebuild:rbac") {
		gk::v1::RebuildRbacCacheEventPayload payload;
		if (!event.payload().UnpackTo(&payload)) {
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
} // namespace svc
