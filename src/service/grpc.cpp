#include "grpc.h"

#include "err/errors.h"

#include "mappers.h"

namespace service {
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
} // namespace service
