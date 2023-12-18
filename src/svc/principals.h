#pragma once

#include <google/rpc/status.pb.h>

#include "db/principals.h"
#include "sentium/api/v1/principals.grpcxx.pb.h"

namespace svc {
using namespace sentium::api::v1::Principals;

class PrincipalsImpl {
public:
	using service_type = Service;

	template <typename T>
	typename T::result_type call(grpcxx::context &, const typename T::request_type &) {
		return {grpcxx::status::code_t::unimplemented, std::nullopt};
	}

	template <>
	rpcCreate::result_type call<rpcCreate>(
		grpcxx::context &ctx, const rpcCreate::request_type &req);

	google::rpc::Status exception() noexcept;

private:
	db::Principal            map(const rpcCreate::request_type &from) const noexcept;
	rpcCreate::response_type map(const db::Principal &from) const noexcept;
};
} // namespace svc
