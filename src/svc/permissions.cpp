#include "permissions.h"

#include "err/errors.h"

namespace svc {
grpc::ServerUnaryReactor *Permissions::Create(
	grpc::CallbackServerContext *context, const gk::v1::PermissionsCreateRequest *request,
	gk::v1::Permission *response) {
	auto *reactor = context->DefaultReactor();

	if (request->id() == "") {
		reactor->Finish(
			grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Permission id cannot be empty"));
		return reactor;
	}

	auto permission = map(request);
	try {
		permission.store();
	} catch (const err::DatastoreDuplicatePermission &e) {
		reactor->Finish(grpc::Status(grpc::StatusCode::ALREADY_EXISTS, e.what()));
		return reactor;
	} catch (...) {
		reactor->Finish(grpc::Status(grpc::StatusCode::UNAVAILABLE, "Failed to store data"));
		return reactor;
	}

	map(permission, response);

	reactor->Finish(grpc::Status::OK);
	return reactor;
}

datastore::Permission Permissions::map(const gk::v1::PermissionsCreateRequest *from) {
	datastore::Permission permission({
		.id = from->id(),
	});

	return permission;
}

void Permissions::map(const datastore::Permission &from, gk::v1::Permission *to) {
	to->set_id(from.id());
}
} // namespace svc
