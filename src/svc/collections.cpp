#include "collections.h"

#include "datastore/access-policies.h"
#include "datastore/identities.h"
#include "datastore/permissions.h"
#include "datastore/rbac-policies.h"
#include "datastore/roles.h"
#include "err/errors.h"

#include "identities.h"

namespace svc {
grpc::ServerUnaryReactor *Collections::AddMember(
	grpc::CallbackServerContext *context, const gk::v1::CollectionsAddMemberRequest *request,
	gk::v1::CollectionsAddMemberResponse *response) {
	auto *reactor = context->DefaultReactor();

	// TODO: error handling
	auto collection = datastore::RetrieveCollection(request->collection_id());

	try {
		collection.add(request->identity_id());
	} catch (const err::DatastoreInvalidCollectionOrMember &) {
		reactor->Finish(
			grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "Invalid collection or member"));
		return reactor;
	} catch (const err::DatastoreDuplicateCollectionMember &) {
		reactor->Finish(
			grpc::Status(grpc::StatusCode::ALREADY_EXISTS, "Member already in the collection"));
		return reactor;
	} catch (...) {
		reactor->Finish(
			grpc::Status(grpc::StatusCode::UNAVAILABLE, "Failed to add identity to collection"));
		return reactor;
	}

	// Update access policies
	auto access = datastore::RetrieveAccessPoliciesByCollection(collection.id());
	for (const auto &policy : access) {
		for (const auto &rule : policy.rules()) {
			datastore::AccessPolicy::Cache cache({
				.identity = request->identity_id(),
				.policy   = policy.id(),
				.rule     = rule,
			});

			cache.store();
		}
	}

	// Update rbac rules
	auto rbac = datastore::RetrieveRbacPoliciesByCollection(collection.id());
	for (const auto &policy : rbac) {
		for (const auto &rule : policy.rules()) {
			const auto role = datastore::RetrieveRole(rule.roleId);
			for (const auto &perm : datastore::RetrievePermissionsByRole(role.id())) {
				datastore::RbacPolicy::Cache cache({
					.identity   = request->identity_id(),
					.permission = perm.id(),
					.policy     = policy.id(),
					.rule       = rule,
				});

				cache.store();
			}
		}
	}

	reactor->Finish(grpc::Status::OK);
	return reactor;
}

grpc::ServerUnaryReactor *Collections::Create(
	grpc::CallbackServerContext *context, const gk::v1::CollectionsCreateRequest *request,
	gk::v1::Collection *response) {
	auto *reactor = context->DefaultReactor();

	if (request->has_id()) {
		try {
			auto col = datastore::RetrieveCollection(request->id());

			reactor->Finish(
				grpc::Status(grpc::StatusCode::ALREADY_EXISTS, "Duplicate collection id"));
			return reactor;
		} catch (const err::DatastoreCollectionNotFound &) {
			// Collection with an `id` matching the request `id` doesn't exist, we can continue with
			// creating a new one.
		} catch (...) {
			reactor->Finish(grpc::Status(grpc::StatusCode::UNAVAILABLE, "Failed to retrieve data"));
			return reactor;
		}
	}

	auto collection = map(request);
	try {
		collection.store();
	} catch (...) {
		reactor->Finish(grpc::Status(grpc::StatusCode::UNAVAILABLE, "Failed to store data"));
		return reactor;
	}

	map(collection, response);

	reactor->Finish(grpc::Status::OK);
	return reactor;
}

grpc::ServerUnaryReactor *Collections::ListMembers(
	grpc::CallbackServerContext *context, const gk::v1::CollectionsListMembersRequest *request,
	gk::v1::CollectionsListMembersResponse *response) {
	auto *reactor = context->DefaultReactor();

	// TODO: error handling
	// TODO: pagination and response metadata
	auto collection = datastore::RetrieveCollection(request->id());
	auto memberIds  = collection.members();

	for (const auto &id : memberIds) {
		auto identity   = datastore::RetrieveIdentity(id);
		auto pbIdentity = response->add_data();

		Identities::map(identity, pbIdentity);
	}

	reactor->Finish(grpc::Status::OK);
	return reactor;
}

grpc::ServerUnaryReactor *Collections::RemoveMember(
	grpc::CallbackServerContext *context, const gk::v1::CollectionsRemoveMemberRequest *request,
	gk::v1::CollectionsRemoveMemberResponse *response) {
	auto *reactor = context->DefaultReactor();

	// TODO: error handling
	auto collection = datastore::RetrieveCollection(request->collection_id());
	collection.remove(request->identity_id());

	reactor->Finish(grpc::Status::OK);
	return reactor;
}

grpc::ServerUnaryReactor *Collections::Retrieve(
	grpc::CallbackServerContext *context, const gk::v1::CollectionsRetrieveRequest *request,
	gk::v1::Collection *response) {
	auto *reactor = context->DefaultReactor();

	try {
		auto collection = datastore::RetrieveCollection(request->id());
		map(collection, response);
	} catch (const err::DatastoreCollectionNotFound &) {
		reactor->Finish(grpc::Status(grpc::StatusCode::NOT_FOUND, "Document not found"));
		return reactor;
	} catch (...) {
		reactor->Finish(grpc::Status(grpc::StatusCode::UNAVAILABLE, "Failed to retrieve data"));
		return reactor;
	}

	reactor->Finish(grpc::Status::OK);
	return reactor;
}

grpc::ServerUnaryReactor *Collections::Update(
	grpc::CallbackServerContext *context, const gk::v1::CollectionsUpdateRequest *request,
	gk::v1::Collection *response) {
	auto *reactor = context->DefaultReactor();

	if (!request->has_name()) {
		// No fields to update -> throw an error
		reactor->Finish(grpc::Status(grpc::StatusCode::INTERNAL, "No fields to update"));
		return reactor;
	}

	std::optional<datastore::Collection> collection = std::nullopt;
	try {
		collection = datastore::RetrieveCollection(request->id());
	} catch (const err::DatastoreCollectionNotFound &) {
		reactor->Finish(grpc::Status(grpc::StatusCode::NOT_FOUND, "Document not found"));
		return reactor;
	} catch (...) {
		reactor->Finish(grpc::Status(grpc::StatusCode::UNAVAILABLE, "Failed to retrieve data"));
		return reactor;
	}

	if (request->has_name()) {
		collection->name(request->name());
	}

	try {
		collection->store();
	} catch (const err::DatastoreRevisionMismatch &) {
		reactor->Finish(grpc::Status(grpc::StatusCode::INTERNAL, "Revision mismatch"));
		return reactor;
	} catch (...) {
		reactor->Finish(grpc::Status(grpc::StatusCode::UNAVAILABLE, "Failed to store data"));
		return reactor;
	}

	map(*collection, response);
	reactor->Finish(grpc::Status::OK);
	return reactor;
}

datastore::Collection Collections::map(const gk::v1::CollectionsCreateRequest *from) {
	return {{
		.id   = from->id(),
		.name = from->name(),
	}};
}

void Collections::map(const datastore::Collection &from, gk::v1::Collection *to) {
	to->set_id(from.id());
	to->set_name(from.name());
}
} // namespace svc
