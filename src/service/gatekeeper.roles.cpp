#include "gatekeeper.h"

#include "mappers.h"

namespace service {
grpc::ServerUnaryReactor *Gatekeeper::CreateRole(
	grpc::CallbackServerContext *context, const gk::v1::CreateRoleRequest *request,
	gk::v1::Role *response) {
	auto *reactor = context->DefaultReactor();

	// TODO: error handling
	auto role = map(request);
	role.store();

	map(role, response);

	reactor->Finish(grpc::Status::OK);
	return reactor;
}

grpc::ServerUnaryReactor *Gatekeeper::RetrieveRole(
	grpc::CallbackServerContext *context, const gk::v1::RetrieveRoleRequest *request,
	gk::v1::Role *response) {
	auto *reactor = context->DefaultReactor();

	// TODO: error handling
	auto role = datastore::RetrieveRole(request->id());
	map(role, response);

	reactor->Finish(grpc::Status::OK);
	return reactor;
}
} // namespace service
