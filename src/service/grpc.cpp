#include "grpc.h"

namespace service {
grpc::ServerUnaryReactor *Grpc::CreateIdentity(
	grpc::CallbackServerContext *context, const gk::v1::CreateIdentityRequest *request,
	gk::v1::Identity *response) {
	auto *reactor = context->DefaultReactor();

	reactor->Finish(grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Not implemented."));
	return reactor;
}
} // namespace service
