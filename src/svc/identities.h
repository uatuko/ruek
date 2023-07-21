#pragma once

#include "datastore/identities.h"
#include "gk/v1/identities.grpc.pb.h"

namespace svc {
class Identities final : public gk::v1::Identities::CallbackService {
public:
	grpc::ServerUnaryReactor *Create(
		grpc::CallbackServerContext *context, const gk::v1::IdentitiesCreateRequest *request,
		gk::v1::Identity *response) override;

	grpc::ServerUnaryReactor *Retrieve(
		grpc::CallbackServerContext *context, const gk::v1::IdentitiesRetrieveRequest *request,
		gk::v1::Identity *response) override;

	grpc::ServerUnaryReactor *Update(
		grpc::CallbackServerContext *context, const gk::v1::IdentitiesUpdateRequest *request,
		gk::v1::Identity *response) override;

	static datastore::Identity map(const gk::v1::IdentitiesCreateRequest *from);

	static void map(const datastore::Identity &from, gk::v1::Identity *to);
};
} // namespace svc
