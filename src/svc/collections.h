#pragma once

#include "datastore/collections.h"
#include "gk/v1/collections.grpc.pb.h"

namespace svc {
class Collections final : public gk::v1::Collections::CallbackService {
public:
	grpc::ServerUnaryReactor *AddMember(
		grpc::CallbackServerContext *context, const gk::v1::CollectionsAddMemberRequest *request,
		gk::v1::CollectionsAddMemberResponse *response) override;

	grpc::ServerUnaryReactor *Create(
		grpc::CallbackServerContext *context, const gk::v1::CollectionsCreateRequest *request,
		gk::v1::Collection *response) override;

	grpc::ServerUnaryReactor *ListMembers(
		grpc::CallbackServerContext *context, const gk::v1::CollectionsListMembersRequest *request,
		gk::v1::CollectionsListMembersResponse *response) override;

	grpc::ServerUnaryReactor *RemoveMember(
		grpc::CallbackServerContext *context, const gk::v1::CollectionsRemoveMemberRequest *request,
		gk::v1::CollectionsRemoveMemberResponse *response) override;

	grpc::ServerUnaryReactor *Retrieve(
		grpc::CallbackServerContext *context, const gk::v1::CollectionsRetrieveRequest *request,
		gk::v1::Collection *response) override;

	grpc::ServerUnaryReactor *Update(
		grpc::CallbackServerContext *context, const gk::v1::CollectionsUpdateRequest *request,
		gk::v1::Collection *response) override;

	static datastore::Collection map(const gk::v1::CollectionsCreateRequest *from);

	static void map(const datastore::Collection &from, gk::v1::Collection *to);
};
} // namespace svc
