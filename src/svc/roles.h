#pragma once

#include "datastore/roles.h"
#include "gk/v1/roles.grpc.pb.h"

namespace svc {
class Roles final : public gk::v1::Roles::CallbackService {
public:
	grpc::ServerUnaryReactor *Create(
		grpc::CallbackServerContext *context, const gk::v1::CreateRoleRequest *request,
		gk::v1::Role *response) override;

	grpc::ServerUnaryReactor *Retrieve(
		grpc::CallbackServerContext *context, const gk::v1::RetrieveRoleRequest *request,
		gk::v1::Role *response) override;

	static datastore::Role map(const gk::v1::CreateRoleRequest *from);

	static void map(const datastore::Role &from, gk::v1::Role *to);
};
} // namespace svc
