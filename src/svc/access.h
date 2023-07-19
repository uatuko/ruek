#pragma once

#include "datastore/access-policies.h"
#include "gk/v1/access.grpc.pb.h"

namespace svc {
class Access final : public gk::v1::Access::CallbackService {
public:
	grpc::ServerUnaryReactor *AddPolicyCollection(
		grpc::CallbackServerContext                    *context,
		const gk::v1::AddAccessPolicyCollectionRequest *request,
		gk::v1::AddAccessPolicyCollectionResponse      *response) override;

	grpc::ServerUnaryReactor *AddPolicyIdentity(
		grpc::CallbackServerContext *context, const gk::v1::AddAccessPolicyIdentityRequest *request,
		gk::v1::AddAccessPolicyIdentityResponse *response) override;

	grpc::ServerUnaryReactor *Check(
		grpc::CallbackServerContext *context, const gk::v1::CheckAccessRequest *request,
		gk::v1::CheckAccessResponse *response) override;

	grpc::ServerUnaryReactor *CreatePolicy(
		grpc::CallbackServerContext *context, const gk::v1::CreateAccessPolicyRequest *request,
		gk::v1::AccessPolicy *response) override;

	grpc::ServerUnaryReactor *RetrievePolicy(
		grpc::CallbackServerContext *context, const gk::v1::RetrieveAccessPolicyRequest *request,
		gk::v1::AccessPolicy *response) override;

	static datastore::AccessPolicy map(const gk::v1::CreateAccessPolicyRequest *from);

	static void map(const datastore::AccessPolicy &from, gk::v1::AccessPolicy *to);
	static void map(const datastore::Policies &from, gk::v1::CheckAccessResponse *to);
};
} // namespace svc
