#include "principals.h"

#include <google/protobuf/util/json_util.h>

namespace svc {
Principals::Principals() : _svc(*this) {}

template <> rpcCreate::result_type Principals::call<rpcCreate>(const rpcCreate::request_type &req) {
	auto p = map(req);
	p.store();

	return {grpcxx::status::code_t::ok, map(p)};
}

db::Principal Principals::map(const rpcCreate::request_type &from) const noexcept {
	db::Principal to({
		.id = from.id(),
	});

	if (from.has_attrs()) {
		std::string attrs;
		google::protobuf::util::MessageToJsonString(from.attrs(), &attrs);

		to.attrs(std::move(attrs));
	}

	if (from.has_parent_id()) {
		to.parentId(from.parent_id());
	}

	return to;
}

rpcCreate::response_type Principals::map(const db::Principal &from) const noexcept {
	rpcCreate::response_type to;
	to.set_id(from.id());

	if (from.attrs()) {
		google::protobuf::util::JsonStringToMessage(*from.attrs(), to.mutable_attrs());
	}

	if (from.parentId()) {
		to.set_parent_id(*from.parentId());
	}

	return to;
}
} // namespace svc
