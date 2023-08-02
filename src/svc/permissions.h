#pragma once

#include "datastore/permissions.h"
#include "gk/v1/permissions.grpc.pb.h"

namespace svc {
class Permissions final : public gk::v1::Permissions::CallbackService {
public:
	grpc::ServerUnaryReactor *Create(
		grpc::CallbackServerContext *context, const gk::v1::PermissionsCreateRequest *request,
		gk::v1::Permission *response) override;

	static datastore::Permission map(const gk::v1::PermissionsCreateRequest *from);

	static void map(const datastore::Permission &from, gk::v1::Permission *to);
};
} // namespace svc
