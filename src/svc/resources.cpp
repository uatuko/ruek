#include "resources.h"

#include <google/protobuf/util/json_util.h>
#include <google/rpc/code.pb.h>

#include "encoding/b32.h"
#include "sentium/detail/pagination.pb.h"

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

	std::uint16_t limit = 30;
	if (req.pagination_limit() > 0 && req.pagination_limit() < 30) {
		limit = req.pagination_limit();
	}

	auto results = db::ListRecordsByPrincipal(
		ctx.meta("space-id"), req.principal_id(), req.resource_type(), lastId, limit);

	auto response = map<rpcList::response_type>(results);
	if (results.size() == limit) {
		sentium::detail::PaginationToken pbToken;
		pbToken.set_last_id(results.back().resourceId());

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

	std::uint16_t limit = 30;
	if (req.pagination_limit() > 0 && req.pagination_limit() < 30) {
		limit = req.pagination_limit();
	}

	auto results = db::ListRecordsByResource(
		ctx.meta("space-id"), req.resource_type(), req.resource_id(), lastId, limit);
	auto response = map<rpcListPrincipals::response_type>(results);

	if (results.size() == limit) {
		sentium::detail::PaginationToken pbToken;
		pbToken.set_last_id(results.back().principalId());

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

template <> rpcList::response_type Impl::map(const db::Records &from) const noexcept {
	rpcList::response_type to;

	auto *arr = to.mutable_resources();
	arr->Reserve(from.size());
	for (const auto &r : from) {
		arr->Add(map<sentium::api::v1::Resource>(r));
	}

	return to;
}

template <> rpcListPrincipals::response_type Impl::map(const db::Records &from) const noexcept {
	rpcListPrincipals::response_type to;

	auto *arr = to.mutable_principals();
	arr->Reserve(from.size());
	for (const auto &r : from) {
		arr->Add(map<sentium::api::v1::ResourcesPrincipal>(r));
	}

	return to;
}

template <> sentium::api::v1::Resource Impl::map(const db::Record &from) const noexcept {
	sentium::api::v1::Resource to;
	to.set_id(from.resourceId());
	to.set_type(from.resourceType());

	if (from.attrs()) {
		google::protobuf::util::JsonStringToMessage(*from.attrs(), to.mutable_attrs());
	}

	return to;
}

template <> sentium::api::v1::ResourcesPrincipal Impl::map(const db::Record &from) const noexcept {
	sentium::api::v1::ResourcesPrincipal to;
	to.set_id(from.principalId());

	if (from.attrs()) {
		google::protobuf::util::JsonStringToMessage(*from.attrs(), to.mutable_attrs());
	}

	return to;
}
} // namespace resources
} // namespace svc
