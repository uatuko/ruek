#include "mappers.h"

#include <google/protobuf/util/json_util.h>

namespace service {
datastore::Collection map(const gk::v1::CreateCollectionRequest *from) {
	return {{
		.id   = from->id(),
		.name = from->name(),
	}};
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
} // namespace service
