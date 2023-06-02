#include "grpc.h"

#include "err/errors.h"

#include "mappers.h"

namespace service {
grpc::ServerUnaryReactor *Grpc::CreateIdentity(
	grpc::CallbackServerContext *context, const gk::v1::CreateIdentityRequest *request,
	gk::v1::Identity *response) {
	auto *reactor = context->DefaultReactor();

	auto identity = map(request);
	try {
		identity.store();
	} catch (const err::DatastoreDuplicateIdentity &e) {
		reactor->Finish(grpc::Status(grpc::StatusCode::ALREADY_EXISTS, e.what()));
		return reactor;
	} catch (...) {
		reactor->Finish(grpc::Status(grpc::StatusCode::UNAVAILABLE, "Failed to store data."));
		return reactor;
	}

	map(identity, response);

	reactor->Finish(grpc::Status::OK);
	return reactor;
}
} // namespace service
