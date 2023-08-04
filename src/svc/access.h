#pragma once

#include "datastore/access-policies.h"
#include "gk/v1/access.grpc.pb.h"

namespace svc {
class Access final : public gk::v1::Access::CallbackService {
public:
	grpc::ServerUnaryReactor *AddPolicyCollection(
		grpc::CallbackServerContext                    *context,
		const gk::v1::AccessAddPolicyCollectionRequest *request,
		gk::v1::AccessAddPolicyCollectionResponse      *response) override;

	grpc::ServerUnaryReactor *AddPolicyIdentity(
		grpc::CallbackServerContext *context, const gk::v1::AccessAddPolicyIdentityRequest *request,
		gk::v1::AccessAddPolicyIdentityResponse *response) override;

	grpc::ServerUnaryReactor *Check(
		grpc::CallbackServerContext *context, const gk::v1::AccessCheckRequest *request,
		gk::v1::AccessCheckResponse *response) override;

	grpc::ServerUnaryReactor *CreatePolicy(
		grpc::CallbackServerContext *context, const gk::v1::AccessCreatePolicyRequest *request,
		gk::v1::AccessPolicy *response) override;

	grpc::ServerUnaryReactor *RemovePolicyIdentity(
		grpc::CallbackServerContext                     *context,
		const gk::v1::AccessRemovePolicyIdentityRequest *request,
		google::protobuf::Empty                         *response) override;

	grpc::ServerUnaryReactor *RetrievePolicy(
		grpc::CallbackServerContext *context, const gk::v1::AccessRetrievePolicyRequest *request,
		gk::v1::AccessPolicy *response) override;

	static datastore::AccessPolicy map(const gk::v1::AccessCreatePolicyRequest *from);

	static void map(const datastore::AccessPolicy &from, gk::v1::AccessPolicy *to);
	static void map(const datastore::Policies &from, gk::v1::AccessCheckResponse *to);
};
} // namespace svc
