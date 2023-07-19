#include "gatekeeper.h"

#include "datastore/rbac-policies.h"
#include "err/errors.h"

#include "mappers.h"

namespace service {
grpc::ServerUnaryReactor *Gatekeeper::CreateCollection(
	grpc::CallbackServerContext *context, const gk::v1::CreateCollectionRequest *request,
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

grpc::ServerUnaryReactor *Gatekeeper::RetrieveCollection(
	grpc::CallbackServerContext *context, const gk::v1::RetrieveCollectionRequest *request,
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

grpc::ServerUnaryReactor *Gatekeeper::UpdateCollection(
	grpc::CallbackServerContext *context, const gk::v1::UpdateCollectionRequest *request,
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

// Members
grpc::ServerUnaryReactor *Gatekeeper::AddCollectionMember(
	grpc::CallbackServerContext *context, const gk::v1::AddCollectionMemberRequest *request,
	gk::v1::AddCollectionMemberResponse *response) {
	auto *reactor = context->DefaultReactor();

	// TODO: error handling
	auto collection = datastore::RetrieveCollection(request->collection_id());
	collection.add(request->identity_id());

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
			for (const auto &perm : role.permissions()) {
				datastore::RbacPolicy::Cache cache({
					.identity   = request->identity_id(),
					.permission = perm,
					.policy     = policy.id(),
					.rule       = rule,
				});

				cache.store();
			}
		}
	}

	reactor->Finish(grpc::Status::OK);
	return reactor;
};

grpc::ServerUnaryReactor *Gatekeeper::ListCollectionMembers(
	grpc::CallbackServerContext *context, const gk::v1::ListCollectionMembersRequest *request,
	gk::v1::ListCollectionMembersResponse *response) {
	auto *reactor = context->DefaultReactor();

	// TODO: error handling
	// TODO: pagination and response metadata
	auto collection = datastore::RetrieveCollection(request->id());
	auto memberIds  = collection.members();

	for (const auto &id : memberIds) {
		auto identity   = datastore::RetrieveIdentity(id);
		auto pbIdentity = response->add_data();

		map(identity, pbIdentity);
	}

	reactor->Finish(grpc::Status::OK);
	return reactor;
}

grpc::ServerUnaryReactor *Gatekeeper::RemoveCollectionMember(
	grpc::CallbackServerContext *context, const gk::v1::RemoveCollectionMemberRequest *request,
	gk::v1::RemoveCollectionMemberResponse *response) {
	auto *reactor = context->DefaultReactor();

	// TODO: error handling
	auto collection = datastore::RetrieveCollection(request->collection_id());
	collection.remove(request->identity_id());

	reactor->Finish(grpc::Status::OK);
	return reactor;
}
} // namespace service
