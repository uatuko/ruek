#include "mappers.h"

#include <google/protobuf/util/json_util.h>

namespace service {
datastore::AccessPolicy map(const gk::v1::CreateAccessPolicyRequest *from) {
	datastore::AccessPolicy policy({
		.id   = from->id(),
		.name = from->name(),
	});

	// if (from->rules.()) {
	// 	std::string rules;
	// 	google::protobuf::util::MessageToJsonString(from->rules(), &rules);

	// 	policy.rules(std::move(rules));
	// }

	return policy;
}

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

void map(const datastore::AccessPolicy &from, gk::v1::AccessPolicy *to) {
	to->set_id(from.id());
	to->set_name(from.name());
}

datastore::Role map(const gk::v1::CreateRoleRequest *from) {
	datastore::Role role({
		.id   = from->id(),
		.name = from->name(),
	});

	if (from->permissions_size() > 0) {
		datastore::Role::permissions_t perms;
		for (int i = 0; i < from->permissions_size(); i++) {
			perms.insert(from->permissions(i));
		}

		role.permissions(std::move(perms));
	}

	return role;
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

void map(const datastore::Role &from, gk::v1::Role *to) {
	to->set_id(from.id());
	to->set_name(from.name());

	for (const auto &perm : from.permissions()) {
		to->add_permissions(perm);
	}
}
} // namespace service
