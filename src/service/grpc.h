#pragma once

#include "gk/v1/gatekeeper.grpc.pb.h"

namespace service {
class Grpc final : public gk::v1::Gatekeeper::CallbackService {
public:
	grpc::ServerUnaryReactor *CreateIdentity(
		grpc::CallbackServerContext *context, const gk::v1::CreateIdentityRequest *request,
		gk::v1::Identity *response) override;
};
} // namespace service
