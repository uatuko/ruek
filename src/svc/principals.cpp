#include "principals.h"

#include <google/protobuf/util/json_util.h>

#include "err/errors.h"

namespace svc {
template <>
rpcCreate::result_type PrincipalsImpl::call<rpcCreate>(
	grpcxx::context &ctx, const rpcCreate::request_type &req) {
	auto p = map(req);
	p.store();

	return {grpcxx::status::code_t::ok, map(p)};
}

grpcxx::status PrincipalsImpl::exception() noexcept {
	try {
		std::rethrow_exception(std::current_exception());
	} catch (const err::DbInvalidPrincipalData &) {
		return grpcxx::status::code_t::invalid_argument;
	} catch (const err::DbInvalidPrincipalParentId &) {
		return grpcxx::status::code_t::invalid_argument;
	} catch (...) {
		return grpcxx::status::code_t::internal;
	}

	return grpcxx::status::code_t::unknown;
}

db::Principal PrincipalsImpl::map(const rpcCreate::request_type &from) const noexcept {
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

rpcCreate::response_type PrincipalsImpl::map(const db::Principal &from) const noexcept {
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
