#include "access.h"

#include "err/errors.h"
#include "logger/logger.h"

namespace svc {
grpc::ServerUnaryReactor *Access::AddPolicyCollection(
	grpc::CallbackServerContext *context, const gk::v1::AccessAddPolicyCollectionRequest *request,
	gk::v1::AccessAddPolicyCollectionResponse *response) {
	auto *reactor = context->DefaultReactor();

	// TODO: error handling
	auto policy = datastore::RetrieveAccessPolicy(request->policy_id());
	policy.addCollection(request->collection_id());

	reactor->Finish(grpc::Status::OK);
	return reactor;
}

grpc::ServerUnaryReactor *Access::AddPolicyIdentity(
	grpc::CallbackServerContext *context, const gk::v1::AccessAddPolicyIdentityRequest *request,
	gk::v1::AccessAddPolicyIdentityResponse *response) {
	auto *reactor = context->DefaultReactor();

	// TODO: error handling
	auto policy = datastore::RetrieveAccessPolicy(request->policy_id());
	policy.addIdentity(request->identity_id());

	reactor->Finish(grpc::Status::OK);
	return reactor;
}

grpc::ServerUnaryReactor *Access::Check(
	grpc::CallbackServerContext *context, const gk::v1::AccessCheckRequest *request,
	gk::v1::AccessCheckResponse *response) {
	auto *reactor = context->DefaultReactor();

	if (request->has_identity_sub()) {
		// TODO: implement checking access by sub
		reactor->Finish(grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Not implemented"));
		return reactor;
	}

	try {
		const auto policies =
			datastore::AccessPolicy::Cache::check(request->identity_id(), request->resource());
		map(policies, response);
	} catch (const std::exception &e) {
		// FIXME: added to help debug unhandled exceptions, should be removed.
		logger::critical("svc", "exception", e.what());
		return reactor;
	} catch (...) {
		reactor->Finish(grpc::Status(grpc::StatusCode::UNAVAILABLE, "Failed to check access"));
		return reactor;
	}

	reactor->Finish(grpc::Status::OK);
	return reactor;
}

grpc::ServerUnaryReactor *Access::CreatePolicy(
	grpc::CallbackServerContext *context, const gk::v1::AccessCreatePolicyRequest *request,
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

	for (const auto &id : request->collection_ids()) {
		policy.addCollection(id);
	}

	for (const auto &id : request->identity_ids()) {
		policy.addIdentity(id);
	}

	map(policy, response);

	reactor->Finish(grpc::Status::OK);
	return reactor;
}

grpc::ServerUnaryReactor *Access::RemovePolicyIdentity(
	grpc::CallbackServerContext *context, const gk::v1::AccessRemovePolicyIdentityRequest *request,
	gk::v1::AccessRemovePolicyIdentityResponse *response) {
	auto *reactor = context->DefaultReactor();

	// TODO: error handling
	auto policy = datastore::RetrieveAccessPolicy(request->policy_id());
	policy.removeIdentity(request->identity_id());

	reactor->Finish(grpc::Status::OK);
	return reactor;
}

grpc::ServerUnaryReactor *Access::RetrievePolicy(
	grpc::CallbackServerContext *context, const gk::v1::AccessRetrievePolicyRequest *request,
	gk::v1::AccessPolicy *response) {
	auto *reactor = context->DefaultReactor();

	try {
		auto policy = datastore::RetrieveAccessPolicy(request->id());
		map(policy, response);
	} catch (const err::DatastoreAccessPolicyNotFound &) {
		reactor->Finish(grpc::Status(grpc::StatusCode::NOT_FOUND, "Policy not found"));
		return reactor;
	} catch (...) {
		reactor->Finish(grpc::Status(grpc::StatusCode::UNAVAILABLE, "Failed to retrieve data"));
		return reactor;
	}

	reactor->Finish(grpc::Status::OK);
	return reactor;
}

datastore::AccessPolicy Access::map(const gk::v1::AccessCreatePolicyRequest *from) {
	datastore::AccessPolicy::Data::rules_t rules;
	for (const auto &rule : from->rules()) {
		std::string attrs;
		google::protobuf::util::MessageToJsonString(rule.attrs(), &attrs);

		rules.insert({
			.attrs    = attrs,
			.resource = rule.resource(),
		});
	}

	datastore::AccessPolicy policy({
		.id    = from->id(),
		.rules = rules,
	});

	if (from->has_name()) {
		policy.name(from->name());
	}

	return policy;
}

void Access::map(const datastore::AccessPolicy &from, gk::v1::AccessPolicy *to) {
	to->set_id(from.id());
	if (from.name()) {
		to->set_name(*from.name());
	}

	for (const auto &rule : from.rules()) {
		auto r = to->mutable_rules()->Add();
		r->set_resource(rule.resource);

		if (!rule.attrs.empty()) {
			google::protobuf::util::JsonStringToMessage(rule.attrs, r->mutable_attrs());
		}
	}

	// Map principals
	for (const auto &id : from.collections()) {
		to->add_collection_ids(id);
	}

	for (const auto &id : from.identities()) {
		to->add_identity_ids(id);
	}
}

void Access::map(const datastore::Policies &from, gk::v1::AccessCheckResponse *to) {
	for (const auto &policy : from) {
		auto p = to->add_policies();
		p->set_id(policy.id);
		if (!policy.attrs.empty()) {
			google::protobuf::util::JsonStringToMessage(policy.attrs, p->mutable_attrs());
		}
	}
}
} // namespace svc
