#include "rbac.h"

#include "err/errors.h"

namespace svc {
grpc::ServerUnaryReactor *Rbac::Check(
	grpc::CallbackServerContext *context, const gk::v1::RbacCheckRequest *request,
	gk::v1::RbacCheckResponse *response) {
	auto *reactor = context->DefaultReactor();

	if (request->has_identity_sub()) {
		// TODO: implement checking access by sub
		reactor->Finish(grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Not implemented"));
		return reactor;
	}

	try {
		const auto policies =
			datastore::RbacPolicy::Cache::check(request->identity_id(), request->permission());
		map(policies, response);
	} catch (...) {
		reactor->Finish(grpc::Status(grpc::StatusCode::UNAVAILABLE, "Failed to check rbac"));
		return reactor;
	}

	reactor->Finish(grpc::Status::OK);
	return reactor;
}

grpc::ServerUnaryReactor *Rbac::CreatePolicy(
	grpc::CallbackServerContext *context, const gk::v1::RbacCreatePolicyRequest *request,
	gk::v1::RbacPolicy *response) {
	auto *reactor = context->DefaultReactor();

	if (request->has_id()) {
		try {
			auto policy = datastore::RetrieveRbacPolicy(request->id());
			reactor->Finish(grpc::Status(grpc::StatusCode::ALREADY_EXISTS, "Duplicate policy id"));
			return reactor;
		} catch (const err::DatastoreRbacPolicyNotFound &) {
			// Policy with an `id` matching the request `id` doesn't exist, we can continue with
			// creating a new one.
		} catch (const std::exception &e) {
			reactor->Finish(grpc::Status(grpc::StatusCode::UNAVAILABLE, "Failed to retrieve data"));
			return reactor;
		}
	}

	auto policy = map(request);
	try {
		// Store the policy
		policy.store();

		// Add rules
		if (request->rules().size() > 0) {
			for (const auto &rule : request->rules()) {
				policy.addRule({.roleId = rule.role_id()});
			}
		}

		// Add principals
		for (const auto &id : request->collection_ids()) {
			policy.addCollection(id);
		}

		for (const auto &id : request->identity_ids()) {
			policy.addIdentity(id);
		}
	} catch (std::exception &e) {
		reactor->Finish(grpc::Status(grpc::StatusCode::UNAVAILABLE, "Failed to store data"));
		return reactor;
	}

	map(policy, response);

	reactor->Finish(grpc::Status::OK);
	return reactor;
}

grpc::ServerUnaryReactor *Rbac::RetrievePolicy(
	grpc::CallbackServerContext *context, const gk::v1::RbacRetrievePolicyRequest *request,
	gk::v1::RbacPolicy *response) {
	auto *reactor = context->DefaultReactor();

	// TODO: error handling
	auto policy = datastore::RetrieveRbacPolicy(request->id());
	map(policy, response);

	reactor->Finish(grpc::Status::OK);
	return reactor;
}

datastore::RbacPolicy Rbac::map(const gk::v1::RbacCreatePolicyRequest *from) {
	datastore::RbacPolicy policy({
		.id = from->id(),
	});

	if (from->has_name()) {
		policy.name(from->name());
	}

	return policy;
}

void Rbac::map(const datastore::Policies &from, gk::v1::RbacCheckResponse *to) {
	for (const auto &policy : from) {
		auto p = to->add_policies();
		p->set_id(policy.id);
		if (!policy.attrs.empty()) {
			google::protobuf::util::JsonStringToMessage(policy.attrs, p->mutable_attrs());
		}
	}
}

void Rbac::map(const datastore::RbacPolicy &from, gk::v1::RbacPolicy *to) {
	to->set_id(from.id());
	if (from.name()) {
		to->set_name(*from.name());
	}

	// Map role ids
	auto rules = from.rules();
	if (rules.size() > 0) {
		for (const auto &rule : rules) {
			auto r = to->add_rules();
			r->set_role_id(rule.roleId);
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
} // namespace svc
