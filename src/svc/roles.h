#pragma once

#include "datastore/permissions.h"
#include "datastore/roles.h"
#include "gk/v1/roles.grpc.pb.h"

#include "permissions.h"
#include "rbac.h"

namespace svc {
class Roles final : public gk::v1::Roles::CallbackService {
public:
	grpc::ServerUnaryReactor *AddPermission(
		grpc::CallbackServerContext *context, const gk::v1::RolesAddPermissionRequest *request,
		gk::v1::Role *response) override;

	grpc::ServerUnaryReactor *Create(
		grpc::CallbackServerContext *context, const gk::v1::RolesCreateRequest *request,
		gk::v1::Role *response) override;

	grpc::ServerUnaryReactor *Retrieve(
		grpc::CallbackServerContext *context, const gk::v1::RolesRetrieveRequest *request,
		gk::v1::Role *response) override;

	static datastore::Role map(const gk::v1::RolesCreateRequest *from);

	static void map(const datastore::Role &from, gk::v1::Role *to);
};
} // namespace svc
