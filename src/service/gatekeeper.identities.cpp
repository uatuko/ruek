#include "gatekeeper.h"

#include "err/errors.h"

#include "mappers.h"

namespace service {
grpc::ServerUnaryReactor *Gatekeeper::CreateIdentity(
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

grpc::ServerUnaryReactor *Gatekeeper::RetrieveIdentity(
	grpc::CallbackServerContext *context, const gk::v1::RetrieveIdentityRequest *request,
	gk::v1::Identity *response) {
	auto *reactor = context->DefaultReactor();

	try {
		if (request->has_sub()) {
			auto identity = datastore::RetrieveIdentityBySub(request->sub());
			map(identity, response);
		} else {
			auto identity = datastore::RetrieveIdentity(request->id());
			map(identity, response);
		}
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

grpc::ServerUnaryReactor *Gatekeeper::UpdateIdentity(
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
} // namespace service
