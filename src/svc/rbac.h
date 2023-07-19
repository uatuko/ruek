#pragma once

#include "datastore/rbac-policies.h"
#include "gk/v1/rbac.grpc.pb.h"

namespace svc {
class Rbac final : public gk::v1::Rbac::CallbackService {
public:
	grpc::ServerUnaryReactor *Check(
		grpc::CallbackServerContext *context, const gk::v1::CheckRbacRequest *request,
		gk::v1::CheckRbacResponse *response) override;

	grpc::ServerUnaryReactor *CreatePolicy(
		grpc::CallbackServerContext *context, const gk::v1::CreateRbacPolicyRequest *request,
		gk::v1::RbacPolicy *response) override;

	static datastore::RbacPolicy map(const gk::v1::CreateRbacPolicyRequest *from);

	static void map(const datastore::Policies &from, gk::v1::CheckRbacResponse *to);
	static void map(const datastore::RbacPolicy &from, gk::v1::RbacPolicy *to);
};
} // namespace svc
