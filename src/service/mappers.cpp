#include "mappers.h"

namespace service {
datastore::Collection map(const gk::v1::CreateCollectionRequest *from) {
	return {{
		.name = from->name(),
	}};
}

datastore::Identity map(const gk::v1::CreateIdentityRequest *from) {
	return {{
		.sub = from->sub(),
	}};
}

void map(const datastore::Collection &from, gk::v1::Collection *to) {
	to->set_id(from.id());
	to->set_name(from.name());
}

void map(const datastore::Identity &from, gk::v1::Identity *to) {
	to->set_id(from.id());
	to->set_sub(from.sub());
}
} // namespace service
