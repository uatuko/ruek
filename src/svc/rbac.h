#pragma once

#include "datastore/rbac-policies.h"
#include "gk/v1/rbac.grpc.pb.h"

namespace svc {
class Rbac final : public gk::v1::Rbac::CallbackService {
public:
	grpc::ServerUnaryReactor *Check(
		grpc::CallbackServerContext *context, const gk::v1::RbacCheckRequest *request,
		gk::v1::RbacCheckResponse *response) override;

	grpc::ServerUnaryReactor *CreatePolicy(
		grpc::CallbackServerContext *context, const gk::v1::RbacCreatePolicyRequest *request,
		gk::v1::RbacPolicy *response) override;

	static datastore::RbacPolicy map(const gk::v1::RbacCreatePolicyRequest *from);

	static void map(const datastore::Policies &from, gk::v1::RbacCheckResponse *to);
	static void map(const datastore::RbacPolicy &from, gk::v1::RbacPolicy *to);
};
} // namespace svc
