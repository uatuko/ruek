#include "resources.h"

#include <google/protobuf/util/json_util.h>
#include <google/rpc/code.pb.h>

#include "encoding/b32.h"
#include "sentium/detail/pagination.pb.h"

#include "common.h"

namespace svc {
namespace resources {
template <>
rpcList::result_type Impl::call<rpcList>(grpcxx::context &ctx, const rpcList::request_type &req) {
	std::string lastId;
	if (req.has_pagination_token()) {
		sentium::detail::PaginationToken pbToken;
		if (pbToken.ParseFromString(encoding::b32::decode(req.pagination_token()))) {
			lastId = pbToken.last_id();
		}
	}

	auto limit = common::pagination_limit_v;
	if (req.pagination_limit() > 0 && req.pagination_limit() < 30) {
		limit = req.pagination_limit();
	}

	db::Tuples results;
	results.reserve(limit);

	while (results.size() < limit) {
		auto tuples = db::ListTuplesRight(
			ctx.meta(common::space_id_v), {req.principal_id()}, {}, lastId, limit);

		if (tuples.empty()) {
			break;
		}

		lastId = tuples.back().rEntityId();
		for (auto &t : tuples) {
			if (t.rEntityType() != req.resource_type()) {
				continue;
			}

			results.push_back(std::move(t));
		}

		if (tuples.size() < limit) {
			break;
		}
	}

	auto response = map<rpcList::response_type>(results);
	if (results.size() == limit) {
		sentium::detail::PaginationToken pbToken;
		pbToken.set_last_id(results.back().rEntityId());

		auto strToken = encoding::b32::encode(pbToken.SerializeAsString());
		response.set_pagination_token(strToken);
	}

	return {grpcxx::status::code_t::ok, response};
}

template <>
rpcListPrincipals::result_type Impl::call<rpcListPrincipals>(
	grpcxx::context &ctx, const rpcListPrincipals::request_type &req) {
	std::string lastId;
	if (req.has_pagination_token()) {
		sentium::detail::PaginationToken pbToken;
		if (pbToken.ParseFromString(encoding::b32::decode(req.pagination_token()))) {
			lastId = pbToken.last_id();
		}
	}

	auto limit = common::pagination_limit_v;
	if (req.pagination_limit() > 0 && req.pagination_limit() < 30) {
		limit = req.pagination_limit();
	}

	db::Tuples results;
	results.reserve(limit);

	while (results.size() < limit) {
		auto tuples = db::ListTuplesLeft(
			ctx.meta(common::space_id_v),
			{req.resource_type(), req.resource_id()},
			{},
			lastId,
			limit);

		if (tuples.empty()) {
			break;
		}

		lastId = tuples.back().lEntityId();
		for (auto &t : tuples) {
			if (!t.lPrincipalId()) {
				continue;
			}

			results.push_back(std::move(t));
		}

		if (tuples.size() < limit) {
			break;
		}
	}

	auto response = map<rpcListPrincipals::response_type>(results);

	if (results.size() == limit) {
		sentium::detail::PaginationToken pbToken;
		pbToken.set_last_id(*results.back().lPrincipalId());

		auto strToken = encoding::b32::encode(pbToken.SerializeAsString());
		response.set_pagination_token(strToken);
	}

	return {grpcxx::status::code_t::ok, response};
}

google::rpc::Status Impl::exception() noexcept {
	google::rpc::Status status;
	status.set_code(google::rpc::UNKNOWN);

	return status;
}

template <> rpcList::response_type Impl::map(const db::Tuples &from) const noexcept {
	rpcList::response_type to;

	auto *arr = to.mutable_resources();
	arr->Reserve(from.size());
	for (const auto &t : from) {
		arr->Add(map<sentium::api::v1::Resource>(t));
	}

	return to;
}

template <> rpcListPrincipals::response_type Impl::map(const db::Tuples &from) const noexcept {
	rpcListPrincipals::response_type to;

	auto *arr = to.mutable_principals();
	arr->Reserve(from.size());
	for (const auto &t : from) {
		arr->Add(map<sentium::api::v1::ResourcesPrincipal>(t));
	}

	return to;
}

template <> sentium::api::v1::Resource Impl::map(const db::Tuple &from) const noexcept {
	sentium::api::v1::Resource to;
	to.set_id(from.rEntityId());
	to.set_type(from.rEntityType());

	if (from.attrs()) {
		google::protobuf::util::JsonStringToMessage(*from.attrs(), to.mutable_attrs());
	}

	return to;
}

template <> sentium::api::v1::ResourcesPrincipal Impl::map(const db::Tuple &from) const noexcept {
	sentium::api::v1::ResourcesPrincipal to;
	to.set_id(*from.lPrincipalId());

	if (from.attrs()) {
		google::protobuf::util::JsonStringToMessage(*from.attrs(), to.mutable_attrs());
	}

	return to;
}
} // namespace resources
} // namespace svc
