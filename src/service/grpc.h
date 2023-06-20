#pragma once

#include "gk/v1/gatekeeper.grpc.pb.h"

namespace service {
class Grpc final : public gk::v1::Gatekeeper::CallbackService {
public:
	grpc::ServerUnaryReactor *CreateCollection(
		grpc::CallbackServerContext *context, const gk::v1::CreateCollectionRequest *request,
		gk::v1::Collection *response) override;

	grpc::ServerUnaryReactor *RetrieveCollection(
		grpc::CallbackServerContext *context, const gk::v1::RetrieveCollectionRequest *request,
		gk::v1::Collection *response) override;

	grpc::ServerUnaryReactor *CreateIdentity(
		grpc::CallbackServerContext *context, const gk::v1::CreateIdentityRequest *request,
		gk::v1::Identity *response) override;
};
} // namespace service
