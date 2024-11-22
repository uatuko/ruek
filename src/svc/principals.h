#pragma once

#include <google/rpc/status.pb.h>

#include "db/principals.h"
#include "ruek/api/v1/principals.grpcxx.pb.h"

namespace svc {
namespace principals {
namespace concepts {
template <typename T>
concept has_mutable_principal = requires(T t) {
	{ t.mutable_principal() } -> std::same_as<ruek::api::v1::Principal *>;
};
} // namespace concepts

using namespace ruek::api::v1::Principals;

class Impl {
public:
	using service_type = Service;

	template <typename T>
	typename T::result_type call(grpcxx::context &, const typename T::request_type &) {
		return {grpcxx::status::code_t::unimplemented, std::nullopt};
	}

	google::rpc::Status exception() noexcept;

private:
	db::Principal map(
		const grpcxx::context &ctx, const rpcCreate::request_type &from) const noexcept;

	rpcList::response_type   map(const db::Principals &from) const noexcept;
	ruek::api::v1::Principal map(const db::Principal &from) const noexcept;

	template <concepts::has_mutable_principal T> T map(const db::Principal &from) const noexcept;
};

template <>
rpcCreate::result_type Impl::call<rpcCreate>(
	grpcxx::context &ctx, const rpcCreate::request_type &req);

template <>
rpcDelete::result_type Impl::call<rpcDelete>(
	grpcxx::context &ctx, const rpcDelete::request_type &req);

template <>
rpcList::result_type Impl::call<rpcList>(grpcxx::context &ctx, const rpcList::request_type &req);

template <>
rpcRetrieve::result_type Impl::call<rpcRetrieve>(
	grpcxx::context &ctx, const rpcRetrieve::request_type &req);

template <>
rpcUpdate::result_type Impl::call<rpcUpdate>(
	grpcxx::context &ctx, const rpcUpdate::request_type &req);
} // namespace principals
} // namespace svc
