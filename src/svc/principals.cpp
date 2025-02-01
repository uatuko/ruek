#include "principals.h"

#include <google/protobuf/util/json_util.h>
#include <google/rpc/code.pb.h>

#include "db/tuples.h"
#include "encoding/b32.h"
#include "err/errors.h"
#include "ruek/detail/pagination.pb.h"

#include "common.h"

namespace svc {
namespace principals {
template <>
rpcCreate::result_type Impl::call<rpcCreate>(
	grpcxx::context &ctx, const rpcCreate::request_type &req) {
	if (req.has_id()) {
		try {
			db::Principal::retrieve(ctx.meta(common::space_id_v), req.id());

			throw err::RpcPrincipalsAlreadyExists();
		} catch (const err::DbPrincipalNotFound &) {
			// Principal doesn't exist, can continue
		}
	}

	auto p = map(ctx, req);
	p.store();

	return {grpcxx::status::code_t::ok, map<rpcCreate::response_type>(p)};
}

template <>
rpcDelete::result_type Impl::call<rpcDelete>(
	grpcxx::context &ctx, const rpcDelete::request_type &req) {

	std::int32_t  cost  = 1;
	std::uint16_t limit = common::cost_limit_v;

	if (req.cost_limit() > 0 && req.cost_limit() <= std::numeric_limits<std::uint16_t>::max()) {
		limit = req.cost_limit();
	}

	db::Tuple::Entity entity(req.id());
	db::Tuples        tuples;

	if (cost < limit) {
		tuples  = db::ListTuplesRight(ctx.meta(common::space_id_v), entity, {}, "", limit - cost);
		cost   += tuples.size();
	}

	if (cost < limit) {
		auto results =
			db::ListTuplesLeft(ctx.meta(common::space_id_v), entity, {}, "", limit - cost);
		cost += results.size();
		tuples.insert(tuples.end(), results.begin(), results.end());
	}

	rpcDelete::response_type response;
	if (cost < limit) {
		auto spaceId = ctx.meta(common::space_id_v);

		if (auto r = db::Principal::discard(spaceId, req.id()); r == false) {
			throw err::RpcPrincipalsNotFound();
		}

		auto r = response.mutable_failed_tuple_ids();
		for (const auto &t : tuples) {
			try {
				db::Tuple::discard(spaceId, t.id());
			} catch (...) {
				*r->Add() = t.id();
			}
		}
	} else {
		cost *= -1;
	}

	response.set_cost(cost);
	return {grpcxx::status::code_t::ok, response};
}

template <>
rpcList::result_type Impl::call<rpcList>(grpcxx::context &ctx, const rpcList::request_type &req) {
	db::Principal::Data::segment_t segment;
	if (req.has_segment()) {
		segment = req.segment();
	}

	std::string lastId;
	if (req.has_pagination_token()) {
		ruek::detail::PaginationToken pbToken;
		if (pbToken.ParseFromString(encoding::b32::decode(req.pagination_token()))) {
			lastId = pbToken.last_id();
		}
	}

	std::uint16_t limit = 30;
	if (req.pagination_limit() > 0 && req.pagination_limit() < 30) {
		limit = req.pagination_limit();
	}

	auto results  = db::ListPrincipals(ctx.meta(common::space_id_v), segment, lastId, limit);
	auto response = map(results);

	if (results.size() == limit) {
		ruek::detail::PaginationToken pbToken;
		pbToken.set_last_id(results.back().id());

		auto strToken = encoding::b32::encode(pbToken.SerializeAsString());
		response.set_pagination_token(strToken);
	}

	return {grpcxx::status::code_t::ok, response};
}

template <>
rpcRetrieve::result_type Impl::call<rpcRetrieve>(
	grpcxx::context &ctx, const rpcRetrieve::request_type &req) {
	auto p = db::Principal::retrieve(ctx.meta(common::space_id_v), req.id());
	return {grpcxx::status::code_t::ok, map<rpcRetrieve::response_type>(p)};
}

template <>
rpcUpdate::result_type Impl::call<rpcUpdate>(
	grpcxx::context &ctx, const rpcUpdate::request_type &req) {
	auto p = db::Principal::retrieve(ctx.meta(common::space_id_v), req.id());
	if (!req.has_attrs() && !req.has_segment()) {
		// Nothing to update
		return {grpcxx::status::code_t::ok, map<rpcUpdate::response_type>(p)};
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
	return {grpcxx::status::code_t::ok, map<rpcUpdate::response_type>(p)};
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

db::Principal Impl::map(
	const grpcxx::context &ctx, const rpcCreate::request_type &from) const noexcept {
	db::Principal to({
		.id      = from.id(),
		.spaceId = std::string(ctx.meta(common::space_id_v)),
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

rpcList::response_type Impl::map(const db::Principals &from) const noexcept {
	rpcList::response_type to;

	auto *arr = to.mutable_principals();
	arr->Reserve(from.size());
	for (const auto &p : from) {
		arr->Add(map(p));
	}

	return to;
}

ruek::api::v1::Principal Impl::map(const db::Principal &from) const noexcept {
	ruek::api::v1::Principal to;
	to.set_id(from.id());

	if (from.attrs()) {
		google::protobuf::util::JsonStringToMessage(*from.attrs(), to.mutable_attrs());
	}

	if (from.segment()) {
		to.set_segment(*from.segment());
	}

	return to;
}

template <concepts::has_mutable_principal T> T Impl::map(const db::Principal &from) const noexcept {
	T to;
	*to.mutable_principal() = map(from);

	return to;
}
} // namespace principals
} // namespace svc
