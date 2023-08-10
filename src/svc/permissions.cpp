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

grpc::ServerUnaryReactor *Permissions::List(
	grpc::CallbackServerContext *context, const gk::v1::PermissionsListRequest *request,
	gk::v1::PermissionsListResponse *response) {
	auto *reactor = context->DefaultReactor();

	// TODO: error handling
	auto permissions = datastore::ListPermissions();

	map(permissions, response);

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

void Permissions::map(const datastore::Permissions &from, gk::v1::PermissionsListResponse *to) {
	for (const auto &perm : from) {
		auto p = to->add_data();
		Permissions::map(perm, p);
	}
}
} // namespace svc
