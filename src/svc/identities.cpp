#include "identities.h"

#include "err/errors.h"

namespace svc {
grpc::ServerUnaryReactor *Identities::Create(
	grpc::CallbackServerContext *context, const gk::v1::IdentitiesCreateRequest *request,
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

grpc::ServerUnaryReactor *Identities::Retrieve(
	grpc::CallbackServerContext *context, const gk::v1::IdentitiesRetrieveRequest *request,
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

grpc::ServerUnaryReactor *Identities::Update(
	grpc::CallbackServerContext *context, const gk::v1::IdentitiesUpdateRequest *request,
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

datastore::Identity Identities::map(const gk::v1::IdentitiesCreateRequest *from) {
	datastore::Identity identity({
		.id  = from->id(),
		.sub = from->sub(),
	});

	if (from->has_attrs()) {
		std::string attrs;
		google::protobuf::util::MessageToJsonString(from->attrs(), &attrs);

		identity.attrs(std::move(attrs));
	}

	return identity;
}

void Identities::map(const datastore::Identity &from, gk::v1::Identity *to) {
	to->set_id(from.id());
	to->set_sub(from.sub());

	if (from.attrs()) {
		google::protobuf::util::JsonStringToMessage(*from.attrs(), to->mutable_attrs());
	}
}
} // namespace svc
