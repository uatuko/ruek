#include "principals.h"

#include <google/protobuf/util/json_util.h>
#include <google/rpc/code.pb.h>

#include "err/errors.h"

namespace svc {
namespace principals {
template <>
rpcCreate::result_type Impl::call<rpcCreate>(
	grpcxx::context &ctx, const rpcCreate::request_type &req) {
	if (req.has_id()) {
		try {
			db::Principal::retrieve(req.id());

			throw err::RpcPrincipalsAlreadyExists();
		} catch (const err::DbPrincipalNotFound &) {
			// Principal doesn't exist, can continue
		}
	}

	auto p = map(req);
	p.store();

	return {grpcxx::status::code_t::ok, map(p)};
}

template <>
rpcDelete::result_type Impl::call<rpcDelete>(
	grpcxx::context &ctx, const rpcDelete::request_type &req) {
	if (auto r = db::Principal::discard(req.id()); r == false) {
		throw err::RpcPrincipalsNotFound();
	}

	return {grpcxx::status::code_t::ok, rpcDelete::response_type()};
}

template <>
rpcList::result_type Impl::call<rpcList>(grpcxx::context &ctx, const rpcList::request_type &req) {
	db::Principal::Data::segment_t segment;
	if (req.has_segment()) {
		segment = req.segment();
	}

	std::string lastId;
	if (req.has_pagination_token()) {
		// FIXME: decode token
		lastId = req.pagination_token();
	}

	uint16_t limit = 30;
	if (req.has_pagination_limit() && req.pagination_limit() < 30) {
		limit = req.pagination_limit();
	}

	auto r = db::ListPrincipals(segment, lastId, limit);
	// FIXME: set pagination_token in response if r.size() == limit
	return {grpcxx::status::code_t::ok, map(r)};
}

template <>
rpcRetrieve::result_type Impl::call<rpcRetrieve>(
	grpcxx::context &ctx, const rpcRetrieve::request_type &req) {
	auto p = db::Principal::retrieve(req.id());
	return {grpcxx::status::code_t::ok, map(p)};
}

template <>
rpcUpdate::result_type Impl::call<rpcUpdate>(
	grpcxx::context &ctx, const rpcUpdate::request_type &req) {
	auto p = db::Principal::retrieve(req.id());
	if (!req.has_attrs() && !req.has_segment()) {
		// Nothing to update
		return {grpcxx::status::code_t::ok, map(p)};
	}

	if (req.has_attrs()) {
		std::string attrs;
		google::protobuf::util::MessageToJsonString(req.attrs(), &attrs);

		p.attrs(std::move(attrs));
	}

	if (req.has_segment()) {
		p.segment(req.segment());
	}

	p.store();
	return {grpcxx::status::code_t::ok, map(p)};
}

google::rpc::Status Impl::exception() noexcept {
	google::rpc::Status status;
	status.set_code(google::rpc::UNKNOWN);

	try {
		std::rethrow_exception(std::current_exception());
	} catch (const err::DbPrincipalInvalidData &e) {
		status.set_code(google::rpc::INVALID_ARGUMENT);
		status.set_message(std::string(e.str()));
	} catch (const err::DbPrincipalNotFound &e) {
		status.set_code(google::rpc::NOT_FOUND);
		status.set_message(std::string(e.str()));
	} catch (const err::DbRevisionMismatch &e) {
		status.set_code(google::rpc::INTERNAL);
		status.set_message(std::string(e.str()));
	} catch (const err::RpcPrincipalsAlreadyExists &e) {
		status.set_code(google::rpc::ALREADY_EXISTS);
		status.set_message(std::string(e.str()));
	} catch (const err::RpcPrincipalsNotFound &e) {
		status.set_code(google::rpc::NOT_FOUND);
		status.set_message(std::string(e.str()));
	} catch (const std::exception &e) {
		status.set_code(google::rpc::INTERNAL);
		status.set_message(e.what());
	}

	return status;
}

db::Principal Impl::map(const rpcCreate::request_type &from) const noexcept {
	db::Principal to({
		.id = from.id(),
	});

	if (from.has_attrs()) {
		std::string attrs;
		google::protobuf::util::MessageToJsonString(from.attrs(), &attrs);

		to.attrs(std::move(attrs));
	}

	if (from.has_segment()) {
		to.segment(from.segment());
	}

	return to;
}

rpcCreate::response_type Impl::map(const db::Principal &from) const noexcept {
	rpcCreate::response_type to;
	to.set_id(from.id());

	if (from.attrs()) {
		google::protobuf::util::JsonStringToMessage(*from.attrs(), to.mutable_attrs());
	}

	if (from.segment()) {
		to.set_segment(*from.segment());
	}

	return to;
}

rpcList::response_type Impl::map(const db::Principals &from) const noexcept {
	rpcList::response_type to;

	auto *arr = to.mutable_principals();
	arr->Reserve(from.size());
	for (const auto &p : from) {
		arr->Add(map(p));
	}

	return to;
}
} // namespace principals
} // namespace svc
