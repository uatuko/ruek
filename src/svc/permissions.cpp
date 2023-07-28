#include "permissions.h"

namespace svc {
grpc::ServerUnaryReactor *Permissions::Create(
	grpc::CallbackServerContext *context, const gk::v1::PermissionsCreateRequest *request,
	gk::v1::Permission *response) {
	auto *reactor = context->DefaultReactor();

	// TODO: error handling
	auto permission = map(request);
	permission.store();

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
