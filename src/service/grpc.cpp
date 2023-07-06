#include "grpc.h"

#include "err/errors.h"

#include "mappers.h"

namespace service {
// Access Policies
grpc::ServerUnaryReactor *Grpc::CheckAccess(
	grpc::CallbackServerContext *context, const gk::v1::CheckAccessRequest *request,
	gk::v1::CheckAccessResponse *response) {
	auto *reactor = context->DefaultReactor();

	try {
		const auto access =
			datastore::AccessPolicy::Record(request->identity_id(), request->resource());
		const auto policies = access.check();
		map(policies, response);
	} catch (...) {
		reactor->Finish(grpc::Status(grpc::StatusCode::UNAVAILABLE, "Failed to check access"));
		return reactor;
	}

	reactor->Finish(grpc::Status::OK);
	return reactor;
}

grpc::ServerUnaryReactor *Grpc::CreateAccessPolicy(
	grpc::CallbackServerContext *context, const gk::v1::CreateAccessPolicyRequest *request,
	gk::v1::AccessPolicy *response) {
	auto *reactor = context->DefaultReactor();

	if (request->has_id()) {
		try {
			auto policy = datastore::RetrieveAccessPolicy(request->id());
			reactor->Finish(grpc::Status(grpc::StatusCode::ALREADY_EXISTS, "Duplicate policy id"));
			return reactor;
		} catch (const err::DatastoreAccessPolicyNotFound &) {
			// Policy with an `id` matching the request `id` doesn't exist, we can continue with
			// creating a new one.
		} catch (...) {
			reactor->Finish(grpc::Status(grpc::StatusCode::UNAVAILABLE, "Failed to retrieve data"));
			return reactor;
		}
	}

	auto policy = map(request);
	try {
		policy.store();
	} catch (...) {
		reactor->Finish(grpc::Status(grpc::StatusCode::UNAVAILABLE, "Failed to store data"));
		return reactor;
	}

	for (const auto &principal : request->principals()) {
		try {
			std::vector<std::string> identities;

			switch (principal.type()) {
			case gk::v1::PrincipalType::collection:
				policy.addCollectionPrincipal(principal.id());
				identities = datastore::ListIdentitiesInCollection(principal.id());
				break;
			case gk::v1::PrincipalType::identity:
				policy.addIdentityPrincipal(principal.id());
				identities.push_back(principal.id());
				break;
			default:
				reactor->Finish(
					grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Unhandled principal"));
				return reactor;
			}

			for (const auto &identity : identities) {
				for (const auto &rule : request->rules()) {
					datastore::AccessPolicy::Record record(identity, rule.resource());
					policy.add(record);
				}
			}
		} catch (...) {
			reactor->Finish(grpc::Status(grpc::StatusCode::UNAVAILABLE, "Failed to add principal"));
			return reactor;
		}
	}

	map(policy, response);

	reactor->Finish(grpc::Status::OK);
	return reactor;
}

// Collections
grpc::ServerUnaryReactor *Grpc::CreateCollection(
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

grpc::ServerUnaryReactor *Grpc::RetrieveCollection(
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

grpc::ServerUnaryReactor *Grpc::UpdateCollection(
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

// Collections - members
grpc::ServerUnaryReactor *Grpc::AddCollectionMember(
	grpc::CallbackServerContext *context, const gk::v1::AddCollectionMemberRequest *request,
	gk::v1::AddCollectionMemberResponse *response) {
	auto *reactor = context->DefaultReactor();

	// TODO: error handling
	auto collection = datastore::RetrieveCollection(request->collection_id());
	collection.add(request->identity_id());

	reactor->Finish(grpc::Status::OK);
	return reactor;
};

grpc::ServerUnaryReactor *Grpc::ListCollectionMembers(
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

grpc::ServerUnaryReactor *Grpc::RemoveCollectionMember(
	grpc::CallbackServerContext *context, const gk::v1::RemoveCollectionMemberRequest *request,
	gk::v1::RemoveCollectionMemberResponse *response) {
	auto *reactor = context->DefaultReactor();

	// TODO: error handling
	auto collection = datastore::RetrieveCollection(request->collection_id());
	collection.remove(request->identity_id());

	reactor->Finish(grpc::Status::OK);
	return reactor;
}

// Identities
grpc::ServerUnaryReactor *Grpc::CreateIdentity(
	grpc::CallbackServerContext *context, const gk::v1::CreateIdentityRequest *request,
	gk::v1::Identity *response) {
	auto *reactor = context->DefaultReactor();

	if (request->has_id()) {
		try {
			auto idn = datastore::RetrieveIdentity(request->id());

			reactor->Finish(
				grpc::Status(grpc::StatusCode::ALREADY_EXISTS, "Duplicate identity id"));
			return reactor;
		} catch (const err::DatastoreIdentityNotFound &) {
			// Identity with an `id` matching the request `id` doesn't exist, we can continue with
			// creating a new one.
		} catch (...) {
			reactor->Finish(grpc::Status(grpc::StatusCode::UNAVAILABLE, "Failed to retrieve data"));
			return reactor;
		}
	}

	auto identity = map(request);
	try {
		identity.store();
	} catch (const err::DatastoreDuplicateIdentity &e) {
		reactor->Finish(grpc::Status(grpc::StatusCode::ALREADY_EXISTS, e.what()));
		return reactor;
	} catch (...) {
		reactor->Finish(grpc::Status(grpc::StatusCode::UNAVAILABLE, "Failed to store data"));
		return reactor;
	}

	map(identity, response);

	reactor->Finish(grpc::Status::OK);
	return reactor;
}

grpc::ServerUnaryReactor *Grpc::RetrieveIdentity(
	grpc::CallbackServerContext *context, const gk::v1::RetrieveIdentityRequest *request,
	gk::v1::Identity *response) {
	auto *reactor = context->DefaultReactor();

	try {
		auto identity = datastore::RetrieveIdentity(request->id());
		map(identity, response);
	} catch (const err::DatastoreIdentityNotFound &) {
		reactor->Finish(grpc::Status(grpc::StatusCode::NOT_FOUND, "Document not found"));
		return reactor;
	} catch (...) {
		reactor->Finish(grpc::Status(grpc::StatusCode::UNAVAILABLE, "Failed to retrieve data"));
		return reactor;
	}

	reactor->Finish(grpc::Status::OK);
	return reactor;
}

grpc::ServerUnaryReactor *Grpc::UpdateIdentity(
	grpc::CallbackServerContext *context, const gk::v1::UpdateIdentityRequest *request,
	gk::v1::Identity *response) {
	auto *reactor = context->DefaultReactor();

	if (!request->has_attrs() && !request->has_sub()) {
		// No fields to update -> throw an error
		reactor->Finish(grpc::Status(grpc::StatusCode::INTERNAL, "No fields to update"));
		return reactor;
	}

	std::optional<datastore::Identity> identity = std::nullopt;
	try {
		identity = datastore::RetrieveIdentity(request->id());
	} catch (const err::DatastoreIdentityNotFound &) {
		reactor->Finish(grpc::Status(grpc::StatusCode::NOT_FOUND, "Document not found"));
		return reactor;
	} catch (...) {
		reactor->Finish(grpc::Status(grpc::StatusCode::UNAVAILABLE, "Failed to retrieve data"));
		return reactor;
	}

	if (request->has_attrs()) {
		std::string attrs;
		google::protobuf::util::MessageToJsonString(request->attrs(), &attrs);

		identity->attrs(std::move(attrs));
	}

	if (request->has_sub()) {
		identity->sub(request->sub());
	}

	try {
		identity->store();
	} catch (const err::DatastoreRevisionMismatch &) {
		reactor->Finish(grpc::Status(grpc::StatusCode::INTERNAL, "Revision mismatch"));
		return reactor;
	} catch (...) {
		reactor->Finish(grpc::Status(grpc::StatusCode::UNAVAILABLE, "Failed to store data"));
		return reactor;
	}

	map(*identity, response);
	reactor->Finish(grpc::Status::OK);
	return reactor;
}

// Roles
grpc::ServerUnaryReactor *Grpc::CreateRole(
	grpc::CallbackServerContext *context, const gk::v1::CreateRoleRequest *request,
	gk::v1::Role *response) {
	auto *reactor = context->DefaultReactor();

	// TODO: error handling
	auto role = map(request);
	role.store();

	map(role, response);

	reactor->Finish(grpc::Status::OK);
	return reactor;
}

grpc::ServerUnaryReactor *Grpc::RetrieveRole(
	grpc::CallbackServerContext *context, const gk::v1::RetrieveRoleRequest *request,
	gk::v1::Role *response) {
	auto *reactor = context->DefaultReactor();

	// TODO: error handling
	auto role = datastore::RetrieveRole(request->id());
	map(role, response);

	reactor->Finish(grpc::Status::OK);
	return reactor;
}
} // namespace service
