#pragma once

#include <google/rpc/status.pb.h>

#include "db/tuples.h"
#include "ruek/api/v1/entities.grpcxx.pb.h"

namespace svc {
namespace entities {
using namespace ruek::api::v1::Entities;

class Impl {
public:
	using service_type = Service;

	template <typename T>
	typename T::result_type call(grpcxx::context &, const typename T::request_type &) {
		return {grpcxx::status::code_t::unimplemented, std::nullopt};
	}

	template <>
	rpcList::result_type call<rpcList>(grpcxx::context &ctx, const rpcList::request_type &req);

	template <>
	rpcListPrincipals::result_type call<rpcListPrincipals>(
		grpcxx::context &ctx, const rpcListPrincipals::request_type &req);

	google::rpc::Status exception() noexcept;

private:
	template <typename T, typename F> T map(const F &) const noexcept;

	template <> rpcList::response_type           map(const db::Tuples &from) const noexcept;
	template <> rpcListPrincipals::response_type map(const db::Tuples &from) const noexcept;

	template <> ruek::api::v1::EntitiesEntity    map(const db::Tuple &from) const noexcept;
	template <> ruek::api::v1::EntitiesPrincipal map(const db::Tuple &from) const noexcept;
};
} // namespace entities
} // namespace svc
