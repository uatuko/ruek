#pragma once

#include "db/principals.h"
#include "sentium/api/v1/principals.grpcxx.pb.h"

namespace svc {
using namespace sentium::api::v1::Principals;

class Principals {
public:
	Principals();

	constexpr auto &svc() noexcept { return _svc; }

	template <typename T> typename T::result_type call(const typename T::request_type &) {
		return {grpcxx::status::code_t::unimplemented, std::nullopt};
	}

	template <> rpcCreate::result_type call<rpcCreate>(const rpcCreate::request_type &req);

private:
	db::Principal            map(const rpcCreate::request_type &from) const noexcept;
	rpcCreate::response_type map(const db::Principal &from) const noexcept;

	Service _svc;
};
} // namespace svc
