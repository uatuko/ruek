#include "mappers.h"

#include <google/protobuf/util/json_util.h>

namespace service {
datastore::Collection map(const gk::v1::CreateCollectionRequest *from) {
	return {{
		.id   = from->id(),
		.name = from->name(),
	}};
}

datastore::Identity map(const gk::v1::CreateIdentityRequest *from) {
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

datastore::RbacPolicy map(const gk::v1::CreateRbacPolicyRequest *from) {
	datastore::RbacPolicy policy({
		.id = from->id(),
	});

	if (from->has_name()) {
		policy.name(from->name());
	}

	return policy;
}

void map(const datastore::AccessPolicy &from, gk::v1::Policy *to) {
	to->set_id(from.id());

	// FIXME: add attributes
}

void map(const datastore::Collection &from, gk::v1::Collection *to) {
	to->set_id(from.id());
	to->set_name(from.name());
}

void map(const datastore::Identity &from, gk::v1::Identity *to) {
	to->set_id(from.id());
	to->set_sub(from.sub());

	if (from.attrs()) {
		google::protobuf::util::JsonStringToMessage(*from.attrs(), to->mutable_attrs());
	}
}

void map(const datastore::Policies &from, gk::v1::CheckRbacResponse *to) {
	for (const auto &policy : from) {
		auto p = to->add_policies();
		p->set_id(policy.id);
	}
}

void map(const datastore::RbacPolicy &from, gk::v1::RbacPolicy *to) {
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
} // namespace service
