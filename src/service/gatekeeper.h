#pragma once

#include "gk/v1/gatekeeper.grpc.pb.h"

namespace service {
class Gatekeeper final : public gk::v1::Gatekeeper::CallbackService {
public:
	// Collections
	grpc::ServerUnaryReactor *CreateCollection(
		grpc::CallbackServerContext *context, const gk::v1::CreateCollectionRequest *request,
		gk::v1::Collection *response) override;

	grpc::ServerUnaryReactor *RetrieveCollection(
		grpc::CallbackServerContext *context, const gk::v1::RetrieveCollectionRequest *request,
		gk::v1::Collection *response) override;

	grpc::ServerUnaryReactor *UpdateCollection(
		grpc::CallbackServerContext *context, const gk::v1::UpdateCollectionRequest *request,
		gk::v1::Collection *response) override;

	// Collections - members
	grpc::ServerUnaryReactor *AddCollectionMember(
		grpc::CallbackServerContext *context, const gk::v1::AddCollectionMemberRequest *request,
		gk::v1::AddCollectionMemberResponse *response) override;

	grpc::ServerUnaryReactor *ListCollectionMembers(
		grpc::CallbackServerContext *context, const gk::v1::ListCollectionMembersRequest *request,
		gk::v1::ListCollectionMembersResponse *response) override;

	grpc::ServerUnaryReactor *RemoveCollectionMember(
		grpc::CallbackServerContext *context, const gk::v1::RemoveCollectionMemberRequest *request,
		gk::v1::RemoveCollectionMemberResponse *response) override;

	// Identities
	grpc::ServerUnaryReactor *CreateIdentity(
		grpc::CallbackServerContext *context, const gk::v1::CreateIdentityRequest *request,
		gk::v1::Identity *response) override;

	grpc::ServerUnaryReactor *RetrieveIdentity(
		grpc::CallbackServerContext *context, const gk::v1::RetrieveIdentityRequest *request,
		gk::v1::Identity *response) override;

	grpc::ServerUnaryReactor *UpdateIdentity(
		grpc::CallbackServerContext *context, const gk::v1::UpdateIdentityRequest *request,
		gk::v1::Identity *response) override;
};
} // namespace service
