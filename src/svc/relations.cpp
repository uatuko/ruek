#include "relations.h"

#include <google/protobuf/util/json_util.h>
#include <google/rpc/code.pb.h>

#include "encoding/b32.h"
#include "err/errors.h"
#include "sentium/detail/pagination.pb.h"

#include "common.h"

namespace svc {
namespace relations {
template <>
rpcCheck::result_type Impl::call<rpcCheck>(
	grpcxx::context &ctx, const rpcCheck::request_type &req) {

	auto strategy = common::strategy_t::direct;
	if (req.has_strategy()) {
		switch (common::strategy_t(req.strategy())) {
		case common::strategy_t::direct:
			strategy = common::strategy_t::direct;
			break;
		case common::strategy_t::set:
			strategy = common::strategy_t::set;
			break;
		default:
			throw err::RpcRelationsInvalidStrategy();
		}
	}

	std::int32_t  cost  = 1;
	std::uint16_t limit = common::cost_limit_v;

	if (req.cost_limit() > 0 && req.cost_limit() <= std::numeric_limits<std::uint16_t>::max()) {
		limit = req.cost_limit();
	}

	db::Tuple::Entity left, right;

	if (req.has_left_principal_id()) {
		left = {req.left_principal_id()};
	} else {
		left = {req.left_entity().type(), req.left_entity().id()};
	}

	if (req.has_right_principal_id()) {
		right = {req.right_principal_id()};
	} else {
		right = {req.right_entity().type(), req.right_entity().id()};
	}

	rpcCheck::response_type response;
	response.set_found(false);

	// Direct strategy
	if (auto tuples = db::LookupTuples(
			ctx.meta(common::space_id_v), left, req.relation(), right, std::nullopt, "", 1);
		!tuples.empty()) {

		response.set_cost(cost);
		response.set_found(true);
		map(tuples.front(), response.mutable_tuple());

		return {grpcxx::status::code_t::ok, response};
	}

	// Set strategy
	if (cost < limit && common::strategy_t::set == strategy) {
		auto t1 = db::ListTuplesRight(ctx.meta(common::space_id_v), left, {}, {}, limit);
		auto t2 =
			db::ListTuplesLeft(ctx.meta(common::space_id_v), right, req.relation(), {}, limit);

		auto i = t1.cbegin();
		auto j = t2.cbegin();
		while (i != t1.cend() && j != t2.cend()) {
			cost++;
			auto r = i->rEntityId().compare(j->lEntityId());

			if (r == 0) {
				if (i->relation() == j->strand() && i->rEntityType() == j->lEntityType()) {
					response.set_found(true);
					map(db::Tuple(*i, *j), response.mutable_tuple());
					break;
				} else {
					i++;
				}
			}

			if (r > 0) {
				i++;
			} else {
				j++;
			}
		}
	}

	response.set_cost(cost);

	return {grpcxx::status::code_t::ok, response};
}

template <>
rpcCreate::result_type Impl::call<rpcCreate>(
	grpcxx::context &ctx, const rpcCreate::request_type &req) {

	auto strategy = common::strategy_t::graph;
	if (req.has_optimize()) {
		switch (common::strategy_t(req.optimize())) {
		case common::strategy_t::direct:
			strategy = common::strategy_t::direct;
			break;
		case common::strategy_t::graph:
			strategy = common::strategy_t::graph;
			break;
		case common::strategy_t::set:
			strategy = common::strategy_t::set;
			break;
		default:
			throw err::RpcRelationsInvalidStrategy();
		}
	}

	auto tuple = map(ctx, req);
	tuple.store();

	rpcCreate::response_type response = map(tuple);

	if (common::strategy_t::graph == strategy) {
		response.set_cost(1);

		return {grpcxx::status::code_t::ok, response};
	}

	// Optimize
	std::int32_t  cost  = 0;
	std::uint16_t limit = common::cost_limit_v;

	if (req.cost_limit() > 0 && req.cost_limit() <= std::numeric_limits<std::uint16_t>::max()) {
		limit = req.cost_limit();
	}

	db::Tuples computed;

	if (tuple.strand() != "" && (common::strategy_t::direct == strategy || tuple.rPrincipalId())) {
		auto results = db::ListTuplesLeft(
			tuple.spaceId(), {tuple.lEntityType(), tuple.lEntityId()}, tuple.strand(), {}, limit);

		cost += results.size();
		for (const auto &r : results) {
			if (common::strategy_t::set == strategy && !r.lPrincipalId()) {
				continue;
			}

			computed.emplace_back(r, tuple);
		}
	}

	if (cost < limit && tuple.relation() != "" &&
		(common::strategy_t::direct == strategy || tuple.lPrincipalId())) {

		auto results = db::ListTuplesRight(
			tuple.spaceId(), {tuple.rEntityType(), tuple.rEntityId()}, {}, {}, limit - cost);

		cost += results.size();
		for (const auto &r : results) {
			if (tuple.relation() != r.strand()) {
				continue;
			}

			if (common::strategy_t::set == strategy && !r.rPrincipalId()) {
				continue;
			}

			computed.emplace_back(tuple, r);
		}
	}

	cost++; // add initial tuple insert cost

	if (cost <= limit) {
		for (db::Tuples::iterator it = computed.begin(); it != computed.end();) {
			try {
				it->store();
				it++;
			} catch (const err::DbTupleAlreadyExists &) {
				// Tuple already exists, don't need the computed entry
				it = computed.erase(it);
			}
		}
	} else {
		cost *= -1;
	}

	map(computed, response.mutable_computed_tuples());
	response.set_cost(cost);

	return {grpcxx::status::code_t::ok, response};
}

template <>
rpcDelete::result_type Impl::call<rpcDelete>(
	grpcxx::context &ctx, const rpcDelete::request_type &req) {
	db::Tuple::Entity left, right;

	if (req.has_left_principal_id()) {
		left = {req.left_principal_id()};
	} else {
		left = {req.left_entity().type(), req.left_entity().id()};
	}

	if (req.has_right_principal_id()) {
		right = {req.right_principal_id()};
	} else {
		right = {req.right_entity().type(), req.right_entity().id()};
	}

	if (auto r = db::Tuple::lookup(
			ctx.meta(common::space_id_v), left, right, req.relation(), req.strand());
		r) {
		db::Tuple::discard(r->id());
	}

	return {grpcxx::status::code_t::ok, rpcDelete::response_type()};
}

template <>
rpcListLeft::result_type Impl::call<rpcListLeft>(
	grpcxx::context &ctx, const rpcListLeft::request_type &req) {

	db::Tuple::Entity right;
	if (req.has_right_principal_id()) {
		right = {req.right_principal_id()};
	} else {
		right = {req.right_entity().type(), req.right_entity().id()};
	}

	std::optional<std::string_view> relation;
	if (req.has_relation()) {
		relation = req.relation();
	}

	std::string lastId;
	if (req.has_pagination_token()) {
		sentium::detail::PaginationToken pbToken;
		if (pbToken.ParseFromString(encoding::b32::decode(req.pagination_token()))) {
			lastId = pbToken.last_id();
		}
	}

	auto limit = common::pagination_limit_v;
	if (req.pagination_limit() > 0 && req.pagination_limit() < limit) {
		limit = req.pagination_limit();
	}

	auto results = db::ListTuplesLeft(ctx.meta(common::space_id_v), right, relation, lastId, limit);

	rpcListLeft::response_type response;
	map(results, response.mutable_tuples());

	if (results.size() == limit) {
		sentium::detail::PaginationToken pbToken;
		pbToken.set_last_id(results.back().lEntityId());

		auto strToken = encoding::b32::encode(pbToken.SerializeAsString());
		response.set_pagination_token(strToken);
	}

	return {grpcxx::status::code_t::ok, response};
}

template <>
rpcListRight::result_type Impl::call<rpcListRight>(
	grpcxx::context &ctx, const rpcListRight::request_type &req) {

	db::Tuple::Entity left;
	if (req.has_left_principal_id()) {
		left = {req.left_principal_id()};
	} else {
		left = {req.left_entity().type(), req.left_entity().id()};
	}

	std::optional<std::string_view> relation;
	if (req.has_relation()) {
		relation = req.relation();
	}

	std::string lastId;
	if (req.has_pagination_token()) {
		sentium::detail::PaginationToken pbToken;
		if (pbToken.ParseFromString(encoding::b32::decode(req.pagination_token()))) {
			lastId = pbToken.last_id();
		}
	}

	auto limit = common::pagination_limit_v;
	if (req.pagination_limit() > 0 && req.pagination_limit() < limit) {
		limit = req.pagination_limit();
	}

	auto results = db::ListTuplesRight(ctx.meta(common::space_id_v), left, relation, lastId, limit);

	rpcListRight::response_type response;
	map(results, response.mutable_tuples());

	if (results.size() == limit) {
		sentium::detail::PaginationToken pbToken;
		pbToken.set_last_id(results.back().rEntityId());

		auto strToken = encoding::b32::encode(pbToken.SerializeAsString());
		response.set_pagination_token(strToken);
	}

	return {grpcxx::status::code_t::ok, response};
}

google::rpc::Status Impl::exception() noexcept {
	google::rpc::Status status;
	status.set_code(google::rpc::UNKNOWN);

	try {
		std::rethrow_exception(std::current_exception());
	} catch (const err::DbTupleInvalidData &e) {
		status.set_code(google::rpc::INVALID_ARGUMENT);
		status.set_message(std::string(e.str()));
	} catch (const err::DbTupleInvalidKey &e) {
		status.set_code(google::rpc::INVALID_ARGUMENT);
		status.set_message(std::string(e.str()));
	} catch (const err::RpcRelationsInvalidStrategy &e) {
		status.set_code(google::rpc::INVALID_ARGUMENT);
		status.set_message(std::string(e.str()));
	} catch (const std::exception &e) {
		status.set_code(google::rpc::INTERNAL);
		status.set_message(e.what());
	}

	return status;
}

db::Tuple Impl::map(
	const grpcxx::context &ctx, const rpcCreate::request_type &from) const noexcept {
	db::Tuple to({
		.lEntityId   = from.left_entity().id(),
		.lEntityType = from.left_entity().type(),
		.relation    = from.relation(),
		.rEntityId   = from.right_entity().id(),
		.rEntityType = from.right_entity().type(),
		.spaceId     = std::string(ctx.meta(common::space_id_v)),
		.strand      = from.strand(),
	});

	if (from.has_attrs()) {
		std::string attrs;
		google::protobuf::util::MessageToJsonString(from.attrs(), &attrs);

		to.attrs(std::move(attrs));
	}

	if (from.has_left_principal_id()) {
		to.lPrincipalId(from.left_principal_id());
	}

	if (from.has_right_principal_id()) {
		to.rPrincipalId(from.right_principal_id());
	}

	return to;
}

rpcCreate::response_type Impl::map(const db::Tuple &from) const noexcept {
	rpcCreate::response_type to;

	auto *tuple = to.mutable_tuple();
	map(from, tuple);

	return to;
}

void Impl::map(const db::Tuple &from, sentium::api::v1::Tuple *to) const noexcept {
	to->set_id(from.id());
	to->set_space_id(from.spaceId());

	if (from.lPrincipalId()) {
		to->set_left_principal_id(*from.lPrincipalId());
	} else {
		auto *entity = to->mutable_left_entity();
		entity->set_id(from.lEntityId());
		entity->set_type(from.lEntityType());
	}

	to->set_relation(from.relation());

	if (from.rPrincipalId()) {
		to->set_right_principal_id(*from.rPrincipalId());
	} else {
		auto *entity = to->mutable_right_entity();
		entity->set_id(from.rEntityId());
		entity->set_type(from.rEntityType());
	}

	if (!from.strand().empty()) {
		to->set_strand(from.strand());
	}

	if (from.attrs()) {
		google::protobuf::util::JsonStringToMessage(*from.attrs(), to->mutable_attrs());
	}

	if (from.ridL()) {
		to->set_ref_id_left(*from.ridL());
	}

	if (from.ridR()) {
		to->set_ref_id_right(*from.ridR());
	}
}

void Impl::map(
	const db::Tuples                                            &from,
	google::protobuf::RepeatedPtrField<sentium::api::v1::Tuple> *to) const noexcept {

	to->Reserve(from.size());
	for (const auto &t : from) {
		map(t, to->Add());
	}
}
} // namespace relations
} // namespace svc
