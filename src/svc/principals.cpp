#include "principals.h"

#include <google/protobuf/util/json_util.h>
#include <google/rpc/code.pb.h>

#include "err/errors.h"

namespace svc {
template <>
rpcCreate::result_type PrincipalsImpl::call<rpcCreate>(
	grpcxx::context &ctx, const rpcCreate::request_type &req) {

	if (req.has_id()) {
		try {
			db::RetrievePrincipal(req.id());

			throw err::RpcPrincipalsAlreadyExists();
		} catch (const err::DbPrincipalNotFound &) {
			// Principal doesn't exist, can continue
		}
	}

	auto p = map(req);
	p.store();

	return {grpcxx::status::code_t::ok, map(p)};
}

google::rpc::Status PrincipalsImpl::exception() noexcept {
	google::rpc::Status status;

	try {
		std::rethrow_exception(std::current_exception());
	} catch (const err::DbInvalidPrincipalData &e) {
		status.set_code(google::rpc::INVALID_ARGUMENT);
		status.set_message(std::string(e.str()));
	} catch (const err::DbInvalidPrincipalParentId &e) {
		status.set_code(google::rpc::INVALID_ARGUMENT);
		status.set_message(std::string(e.str()));
	} catch (const err::DbRevisionMismatch &e) {
		status.set_code(google::rpc::INTERNAL);
		status.set_message(std::string(e.str()));
	} catch (const err::RpcPrincipalsAlreadyExists &e) {
		status.set_code(google::rpc::ALREADY_EXISTS);
		status.set_message(std::string(e.str()));
	} catch (const std::exception &e) {
		status.set_code(google::rpc::INTERNAL);
		status.set_message(e.what());
	} catch (...) {
		status.set_code(google::rpc::UNKNOWN);
	}

	return status;
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
