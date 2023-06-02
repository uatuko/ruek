#include "mappers.h"

namespace service {
datastore::Identity map(const gk::v1::CreateIdentityRequest *from) {
	return {{
		.sub = from->sub(),
	}};
}

void map(const datastore::Identity &from, gk::v1::Identity *to) {
	to->set_id(from.id());
	to->set_sub(from.sub());
}
} // namespace service
