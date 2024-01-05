#pragma once

#include <google/rpc/status.pb.h>

#include "db/records.h"
#include "sentium/api/v1/resources.grpcxx.pb.h"

namespace svc {
namespace resources {
using namespace sentium::api::v1::Resources;

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

	template <> rpcList::response_type           map(const db::Records &from) const noexcept;
	template <> rpcListPrincipals::response_type map(const db::Records &from) const noexcept;

	template <> sentium::api::v1::Resource           map(const db::Record &from) const noexcept;
	template <> sentium::api::v1::ResourcesPrincipal map(const db::Record &from) const noexcept;
};
} // namespace resources
} // namespace svc
