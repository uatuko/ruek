#pragma once

#include "datastore/collections.h"
#include "gk/v1/collections.grpc.pb.h"

namespace svc {
class Collections final : public gk::v1::Collections::CallbackService {
public:
	grpc::ServerUnaryReactor *AddMember(
		grpc::CallbackServerContext *context, const gk::v1::AddCollectionMemberRequest *request,
		gk::v1::AddCollectionMemberResponse *response) override;

	grpc::ServerUnaryReactor *Create(
		grpc::CallbackServerContext *context, const gk::v1::CreateCollectionRequest *request,
		gk::v1::Collection *response) override;

	grpc::ServerUnaryReactor *ListMembers(
		grpc::CallbackServerContext *context, const gk::v1::ListCollectionMembersRequest *request,
		gk::v1::ListCollectionMembersResponse *response) override;

	grpc::ServerUnaryReactor *RemoveMember(
		grpc::CallbackServerContext *context, const gk::v1::RemoveCollectionMemberRequest *request,
		gk::v1::RemoveCollectionMemberResponse *response) override;

	grpc::ServerUnaryReactor *Retrieve(
		grpc::CallbackServerContext *context, const gk::v1::RetrieveCollectionRequest *request,
		gk::v1::Collection *response) override;

	grpc::ServerUnaryReactor *Update(
		grpc::CallbackServerContext *context, const gk::v1::UpdateCollectionRequest *request,
		gk::v1::Collection *response) override;

	static datastore::Collection map(const gk::v1::CreateCollectionRequest *from);

	static void map(const datastore::Collection &from, gk::v1::Collection *to);
};
} // namespace svc
